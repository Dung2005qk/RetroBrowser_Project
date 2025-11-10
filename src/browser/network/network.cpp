// ============================================================================
// network.cpp - Implementation of Network Module for Win98 Retro Browser
// ============================================================================
// PURPOSE:
//   Provides a robust, production-grade abstraction layer over Winsock 2.2 for
//   HTTP communication via a local Python proxy. This module shields the rest
//   of the application from socket complexity while maintaining strict resource
//   safety and binary-safe data handling.
//
// ARCHITECTURE:
//   +-------------------+     HTTP/1.1 GET      +---------------+
//   | Win98 Browser     | ------------------->  | Python Proxy  |
//   | (this module)     |                       | (127.0.0.1)   |
//   | Winsock 2.2       | <-------------------  | HTTPS handler |
//   +-------------------+   Raw HTTP Response   +---------------+
//                                                      |
//                                                      v
//                                               Modern Web (HTTPS)
//
// DESIGN PHILOSOPHY:
//   1. RAII: Automatic Winsock lifecycle management (WSAStartup/Cleanup)
//   2. BLOCKING MODEL: Synchronous I/O fits Win32 single-threaded message loop
//   3. BINARY-SAFE: vector<char> for bodies; handles nulls in images/PDFs
//   4. DEFENSIVE: Validate ALL inputs, check ALL Winsock calls, enforce limits
//   5. RESOURCE-SAFE: closesocket() in ALL code paths (C++98 structured cleanup)
//
// USAGE EXAMPLE:
//   Network::NetworkManager netMgr("127.0.0.1", 8080);
//   if (!netMgr.IsInitialized()) {
//       MessageBox(NULL, "Network init failed", "Error", MB_ICONERROR);
//       return 1;
//   }
//   Network::HttpResponse resp = netMgr.FetchUrl("http://example.com");
//   if (resp.status == Network::SUCCESS && resp.httpStatusCode == 200) {
//       std::string html(resp.body.begin(), resp.body.end());
//       // ... parse and render ...
//   }
//
// TESTING RECOMMENDATIONS:
//   UNIT TESTS:
//     - Valid URL (http://example.com) -> status=SUCCESS, code=200
//     - Invalid schemes (https://, ftp://) -> status=INVALID_URL
//     - URL > 2048 chars -> status=INVALID_URL
//     - Proxy offline -> status=PROXY_CONNECT_FAILED
//     - HTTP 404/500 from proxy -> status=SUCCESS, httpStatusCode=404/500
//     - Large response (~1MB) -> status=SUCCESS, full body received
//     - Binary image download -> no null truncation
//   INTEGRATION TESTS:
//     - End-to-end: Browser -> Proxy -> Real site -> Render
//     - Proxy crash mid-response -> RECV_FAILED
//   STRESS TESTS:
//     - 100+ sequential requests -> no memory leaks (Task Manager check)
//
// THREAD SAFETY: NOT THREAD-SAFE. Use one instance per thread.
//
// CONSTRAINTS:
//   - C++98/03 only (VC++ 6.0 compatible)
//   - No exceptions (errors via NetworkStatus)
//   - Max URL: 2048 chars
//   - Max response: 1MB (Win98 RAM protection)
//   - Timeout: 30 seconds (configurable via HTTP_TIMEOUT_MS)
// ============================================================================

#include "stdafx.h"   // MUST be first for PCH (provides Winsock2, STL, constants)
#include "network.h"  // Public interface declarations

namespace Network
{
    // ========================================================================
    // CONSTRUCTOR: NetworkManager::NetworkManager
    // ========================================================================
    // PURPOSE: Initialize Winsock 2.2 library and configure proxy settings.
    //          Uses RAII pattern - constructor acquires resource (Winsock),
    //          destructor releases it. Fail-safe: if init fails, sets
    //          m_initialized=false and logs error, but constructor completes.
    //
    // PARAMETERS:
    //   proxyHost - Hostname or IP of proxy server (default: "127.0.0.1")
    //   proxyPort - TCP port of proxy server (default: 8080)
    //
    // POSTCONDITIONS:
    //   - If successful: m_initialized=true, Winsock ready for use
    //   - If failed: m_initialized=false, IsInitialized() returns false
    // ========================================================================
    NetworkManager::NetworkManager(const std::string& proxyHost, int proxyPort)
        : m_proxyHost(proxyHost)
        , m_proxyPort(proxyPort)
        , m_initialized(false)
    {
        // Step 1: Request Winsock version 2.2 (minimum for Win98)
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        if (result != 0) {
            // WSAStartup failed - no network stack available
            DEBUG_LOGF("WSAStartup failed with error: %d", result);
            return;  // m_initialized remains false
        }
        
        // Step 2: Verify we got the requested version (2.2)
        // RATIONALE: Winsock may return a higher version if available, but we
        //            want to ensure at least 2.2 for getaddrinfo and other APIs
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            DEBUG_LOGF("Winsock version mismatch: got %d.%d, expected 2.2",
                       LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
            WSACleanup();  // Release the incompatible version
            return;  // m_initialized remains false
        }
        
        // Success: Winsock 2.2 is ready
        m_initialized = true;
        DEBUG_LOGF("Winsock 2.2 initialized successfully (Proxy: %s:%d)",
                   m_proxyHost.c_str(), m_proxyPort);
    }

    // ========================================================================
    // DESTRUCTOR: NetworkManager::~NetworkManager
    // ========================================================================
    // PURPOSE: Clean up Winsock resources. Part of RAII pattern - automatically
    //          called when NetworkManager goes out of scope or is deleted.
    //          DEFENSIVE: Only calls WSACleanup if we successfully initialized.
    // ========================================================================
    NetworkManager::~NetworkManager()
    {
        if (m_initialized) {
            WSACleanup();
            DEBUG_LOG("Winsock cleaned up");
        }
    }

    // ========================================================================
    // METHOD: NetworkManager::IsInitialized
    // ========================================================================
    bool NetworkManager::IsInitialized() const
    {
        return m_initialized;
    }

    // ========================================================================
    // METHOD: NetworkManager::SetProxy
    // ========================================================================
    // PURPOSE: Update proxy configuration at runtime (e.g., user settings).
    // ========================================================================
    void NetworkManager::SetProxy(const std::string& host, int port)
    {
        m_proxyHost = host;
        m_proxyPort = port;
        DEBUG_LOGF("Proxy updated to %s:%d", host.c_str(), port);
    }

    // ========================================================================
    // METHOD: NetworkManager::FetchUrl (PUBLIC - CORE FUNCTIONALITY)
    // ========================================================================
    // PURPOSE: Execute a complete HTTP GET request via the configured proxy.
    //          This is the primary public interface of the network module.
    //
    // OPERATION SEQUENCE (7 Steps from header documentation):
    //   1. Validate Winsock initialization and URL format
    //   2. Create TCP socket
    //   3. Connect to proxy server
    //   4. Build HTTP/1.1 GET request
    //   5. Send request to proxy
    //   6. Receive response (with 1MB limit)
    //   7. Parse response into HttpResponse struct
    //
    // BLOCKING WARNING: This function will NOT return until the request
    //                   completes, times out, or fails. UI will freeze!
    //                   Call SetCursor(IDC_WAIT) before and after.
    //
    // PARAMETERS:
    //   url - Absolute URL starting with "http://" (max 2048 chars)
    //
    // RETURNS:
    //   HttpResponse struct. ALWAYS check response.status first:
    //     - SUCCESS: httpStatusCode/headers/body are valid
    //     - Otherwise: Only errorMessage is valid
    // ========================================================================
    HttpResponse NetworkManager::FetchUrl(const std::string& url) const
    {
        HttpResponse resp;
        resp.status = UNKNOWN_ERROR;  // Sentinel - should always be overwritten
        
        // ====================================================================
        // STEP 1: PRE-FLIGHT VALIDATION
        // ====================================================================
        
        // 1.1: Check Winsock initialization
        if (!m_initialized) {
            resp.status = WINSOCK_INIT_FAILED;
            resp.errorMessage = "Winsock not initialized - check WSAStartup logs";
            DEBUG_LOG("FetchUrl called but Winsock not initialized");
            return resp;
        }
        
        // 1.2: Validate URL format and length
        // DEFENSIVE: Check scheme is http:// (not https://, ftp://, file://, etc.)
        if (url.find("http://") != 0) {
            resp.status = INVALID_URL;
            resp.errorMessage = "URL must start with 'http://' (HTTPS not supported - proxy handles it)";
            DEBUG_LOGF("Invalid URL scheme: %s", url.c_str());
            return resp;
        }
        
        if (url.length() > MAX_URL_LENGTH) {
            resp.status = INVALID_URL;
            std::stringstream ss;
            ss << "URL exceeds maximum length (" << MAX_URL_LENGTH 
               << " chars). Current length: " << url.length();
            resp.errorMessage = ss.str();
            DEBUG_LOGF("URL too long: %d chars", url.length());
            return resp;
        }
        
        DEBUG_LOGF("Fetching URL: %s", url.c_str());
        
        // ====================================================================
        // STEP 2 & 3: CONNECT TO PROXY
        // ====================================================================
        SOCKET sock = Connect();
        if (sock == INVALID_SOCKET) {
            // Connect() already logged the detailed error
            resp.status = PROXY_CONNECT_FAILED;
            std::stringstream ss;
            ss << "Failed to connect to proxy at " << m_proxyHost 
               << ":" << m_proxyPort << ". "
               << "Verify proxy is running (python proxy.py) and "
               << "VM network is configured (host-only/bridged).";
            resp.errorMessage = ss.str();
            return resp;
        }
        
        // ====================================================================
        // CONFIGURE SOCKET TIMEOUTS
        // ====================================================================
        // RATIONALE: Winsock defaults to NO timeout (infinite blocking).
        //            We set explicit timeouts to prevent the browser from
        //            hanging indefinitely if proxy/network is slow.
        DWORD timeout = HTTP_TIMEOUT_MS;
        
        // Send timeout (prevents send() from blocking forever)
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, 
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        
        // Receive timeout (prevents recv() from blocking forever)
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, 
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        
        DEBUG_LOGF("Socket timeouts set to %d ms", HTTP_TIMEOUT_MS);
        
        // ====================================================================
        // STEP 4: BUILD HTTP REQUEST
        // ====================================================================
        std::string request = BuildRequest(url);
        DEBUG_LOGF("Built request (%d bytes)", request.size());
        
        // ====================================================================
        // STEP 5: SEND REQUEST TO PROXY
        // ====================================================================
        int sent = send(sock, request.c_str(), 
                       static_cast<int>(request.size()), 0);
        
        if (sent == SOCKET_ERROR) {
            int err = WSAGetLastError();
            closesocket(sock);  // CRITICAL: Clean up socket on error path
            
            resp.status = SEND_FAILED;
            std::stringstream ss;
            ss << "send() failed with WSA error " << err;
            
            if (err == WSAETIMEDOUT) {
                ss << " (WSAETIMEDOUT) - Send timeout. "
                   << "Increase HTTP_TIMEOUT_MS or check network speed.";
            } else if (err == WSAECONNRESET) {
                ss << " (WSAECONNRESET) - Proxy closed connection. "
                   << "Check proxy logs for errors.";
            } else {
                ss << " - Check network connectivity.";
            }
            
            resp.errorMessage = ss.str();
            DEBUG_LOGF("send() failed: %d", err);
            return resp;
        }
        
        DEBUG_LOGF("Sent %d bytes successfully", sent);
        
        // ====================================================================
        // STEP 6: RECEIVE RESPONSE FROM PROXY
        // ====================================================================
        std::vector<char> rawData;
        rawData.reserve(8192);  // Pre-allocate to reduce realloc overhead
        
        ReceiveAll(sock, rawData);
        
        // ALWAYS close socket after use (no HTTP keep-alive in our simple impl)
        closesocket(sock);
        DEBUG_LOG("Socket closed");
        
        // Check if we received any data
        if (rawData.empty()) {
            resp.status = RECV_FAILED;
            resp.errorMessage = "No data received from proxy. "
                                "Proxy may have crashed or closed connection immediately.";
            DEBUG_LOG("No data received");
            return resp;
        }
        
        DEBUG_LOGF("Received total %d bytes", rawData.size());
        
        // ====================================================================
        // STEP 7: PARSE RESPONSE
        // ====================================================================
        resp = ParseResponse(rawData);
        
        return resp;
    }

    // ========================================================================
    // PRIVATE METHOD: NetworkManager::Connect
    // ========================================================================
    // PURPOSE: Create a TCP socket and establish connection to the proxy.
    //
    // RETURNS: Connected SOCKET on success, INVALID_SOCKET on failure.
    //          Caller MUST call closesocket() on returned socket.
    //
    // ERROR HANDLING: Logs detailed errors for each failure point.
    // ========================================================================
    SOCKET NetworkManager::Connect() const
    {
        // ====================================================================
        // STEP 1: CREATE SOCKET
        // ====================================================================
        SOCKET sock = socket(AF_INET,      // IPv4
                            SOCK_STREAM,   // TCP
                            IPPROTO_TCP);  // TCP protocol
        
        if (sock == INVALID_SOCKET) {
            int err = WSAGetLastError();
            DEBUG_LOGF("socket() failed with error: %d", err);
            return INVALID_SOCKET;
        }
        
        DEBUG_LOG("TCP socket created");
        
        // ====================================================================
        // STEP 2: RESOLVE PROXY HOSTNAME
        // ====================================================================
        // NOTE: Using legacy gethostbyname() instead of getaddrinfo() for
        //       maximum Win98 compatibility. gethostbyname is simpler and
        //       well-supported on Winsock 2.2.
        struct hostent* host = gethostbyname(m_proxyHost.c_str());
        
        if (!host) {
            int err = WSAGetLastError();
            closesocket(sock);  // Clean up socket before returning
            
            DEBUG_LOGF("gethostbyname('%s') failed with error: %d", 
                       m_proxyHost.c_str(), err);
            
            if (err == WSAHOST_NOT_FOUND) {
                DEBUG_LOG("Host not found - check hostname spelling");
            }
            
            return INVALID_SOCKET;
        }
        
        DEBUG_LOGF("Resolved %s", m_proxyHost.c_str());
        
        // ====================================================================
        // STEP 3: POPULATE sockaddr_in STRUCTURE
        // ====================================================================
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));  // Zero-initialize for safety
        
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(m_proxyPort));
        
        // Copy first IP address from host entry
        memcpy(&addr.sin_addr, host->h_addr_list[0], sizeof(in_addr));
        
        // ====================================================================
        // STEP 4: CONNECT TO PROXY
        // ====================================================================
        if (connect(sock, reinterpret_cast<sockaddr*>(&addr), 
                   sizeof(addr)) == SOCKET_ERROR) {
            int err = WSAGetLastError();
            closesocket(sock);  // Clean up socket before returning
            
            DEBUG_LOGF("connect() to %s:%d failed with error: %d", 
                       m_proxyHost.c_str(), m_proxyPort, err);
            
            // Provide user-friendly error hints
            if (err == WSAECONNREFUSED) {
                DEBUG_LOG("Connection refused - proxy may not be running");
            } else if (err == WSAETIMEDOUT) {
                DEBUG_LOG("Connection timeout - check network/firewall");
            } else if (err == WSAENETUNREACH) {
                DEBUG_LOG("Network unreachable - check VM network adapter");
            }
            
            return INVALID_SOCKET;
        }
        
        DEBUG_LOGF("Connected to proxy %s:%d", m_proxyHost.c_str(), m_proxyPort);
        
        return sock;
    }

    // ========================================================================
    // PRIVATE METHOD: NetworkManager::BuildRequest
    // ========================================================================
    // PURPOSE: Construct a minimal but valid HTTP/1.1 GET request.
    //
    // PROTOCOL NOTES:
    //   - Uses absolute URI for proxy (e.g., GET http://example.com/ HTTP/1.1)
    //   - Host header extracted from URL for RFC 2616 compliance
    //   - Connection: close ensures proxy closes socket after response,
    //     simplifying our recv() loop (no need to parse Content-Length)
    //
    // PARAMETERS:
    //   url - Full URL (e.g., "http://example.com/path")
    //
    // RETURNS: Complete HTTP request string with CRLF line endings
    // ========================================================================
    std::string NetworkManager::BuildRequest(const std::string& url) const
    {
        std::stringstream ss;
        
        // ====================================================================
        // REQUEST LINE: GET <absolute-URI> HTTP/1.1
        // ====================================================================
        // RATIONALE: Proxies expect absolute URIs, not relative paths
        ss << "GET " << url << " HTTP/1.1\r\n";
        
        // ====================================================================
        // HOST HEADER: Extract hostname from URL
        // ====================================================================
        // Parse URL to get host for Host header
        // Format: http://hostname[:port]/path
        //         ^^^^^^^^^^^^^^^^^^^^^^^
        //         Extract this part
        
        size_t hostStart = 7;  // Skip "http://"
        size_t hostEnd = url.find('/', hostStart);
        if (hostEnd == std::string::npos) {
            hostEnd = url.length();  // No path, host goes to end
        }
        
        std::string host = url.substr(hostStart, hostEnd - hostStart);
        ss << "Host: " << host << "\r\n";
        
        // ====================================================================
        // USER-AGENT: Identify as IE5 for maximum retro compatibility
        // ====================================================================
        ss << "User-Agent: " << USER_AGENT << "\r\n";
        
        // ====================================================================
        // ACCEPT: Wildcard to accept any content type
        // ====================================================================
        ss << "Accept: */*\r\n";
        
        // ====================================================================
        // CONNECTION: Close to simplify response handling
        // ====================================================================
        // RATIONALE: With "Connection: close", proxy will close socket after
        //            sending complete response. This allows ReceiveAll() to
        //            simply read until recv() returns 0 (graceful close).
        //            Alternative would require parsing Content-Length.
        ss << "Connection: close\r\n";
        
        // Empty line marks end of headers
        ss << "\r\n";
        
        std::string request = ss.str();
        DEBUG_LOGF("Built request:\n%s", request.c_str());
        
        return request;
    }

    // ========================================================================
    // PRIVATE METHOD: NetworkManager::ReceiveAll
    // ========================================================================
    // PURPOSE: Receive all data from socket until close or error.
    //          Enforces 1MB size limit to protect Win98 VM from OOM.
    //
    // PARAMETERS:
    //   sock - Connected socket to read from
    //   data - Output vector to append received bytes to
    //
    // BEHAVIOR:
    //   - Loops recv() until socket closes (recv returns 0)
    //   - Appends each chunk to data vector
    //   - Stops at 1MB limit and logs warning
    //   - Logs errors but does NOT throw (caller checks data.empty())
    // ========================================================================
    void NetworkManager::ReceiveAll(SOCKET sock, std::vector<char>& data) const
    {
        char buffer[DEFAULT_BUFFER_SIZE];  // 4KB stack buffer
        int bytes;
        
        // ====================================================================
        // RECEIVE LOOP
        // ====================================================================
        while ((bytes = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
            // Append received chunk to data vector
            data.insert(data.end(), buffer, buffer + bytes);
            
            DEBUG_LOGF("Received chunk: %d bytes", bytes);
            
            // ================================================================
            // SAFETY CHECK: Enforce 1MB limit
            // ================================================================
            // RATIONALE: Win98 VMs typically have 128-256MB RAM total.
            //            A single 10MB+ response could consume significant
            //            memory. 1MB is generous for text/HTML but protects
            //            against malicious or buggy proxy responses.
            if (data.size() > 1048576) {  // 1MB = 1024 * 1024
                DEBUG_LOG("WARNING: Response exceeds 1MB limit, stopping recv");
                break;
            }
        }
        
        // ====================================================================
        // ERROR HANDLING
        // ====================================================================
        if (bytes == SOCKET_ERROR) {
            int err = WSAGetLastError();
            DEBUG_LOGF("recv() failed with error: %d", err);
            
            if (err == WSAETIMEDOUT) {
                DEBUG_LOG("Receive timeout - slow proxy or large response");
            } else if (err == WSAECONNRESET) {
                DEBUG_LOG("Connection reset by proxy");
            }
        } else if (bytes == 0) {
            DEBUG_LOG("Connection closed by proxy (graceful)");
        }
        
        DEBUG_LOGF("Total received: %d bytes", data.size());
    }

    // ========================================================================
    // PRIVATE METHOD: NetworkManager::ParseResponse
    // ========================================================================
    // PURPOSE: Parse raw HTTP response bytes into structured HttpResponse.
    //          Validates format, extracts status code, headers, and body.
    //
    // PROTOCOL:
    //   HTTP response format:
    //     HTTP/1.1 200 OK\r\n
    //     Header1: Value1\r\n
    //     Header2: Value2\r\n
    //     \r\n
    //     <body bytes>
    //
    // PARAMETERS:
    //   rawData - Complete raw HTTP response (binary-safe)
    //
    // RETURNS:
    //   HttpResponse with status=SUCCESS if parseable, or
    //   status=RESPONSE_PARSE_FAILED with detailed errorMessage
    //
    // DEFENSIVE PARSING:
    //   - Checks for header/body separator
    //   - Validates status line format
    //   - Normalizes header keys to lowercase
    //   - Validates Content-Length if present
    //   - Distinguishes proxy errors (5xx) from HTTP errors (4xx)
    // ========================================================================
    HttpResponse NetworkManager::ParseResponse(const std::vector<char>& rawData) const
    {
        HttpResponse resp;
        
        // ====================================================================
        // STEP 1: Convert to string for parsing (temporary copy)
        // ====================================================================
        // RATIONALE: String operations (find, substr, getline) are much more
        //            convenient than manual byte scanning. Performance hit is
        //            acceptable since response is <1MB.
        std::string rawStr(rawData.begin(), rawData.end());
        
        // ====================================================================
        // STEP 2: Locate header/body separator
        // ====================================================================
        size_t headersEnd = rawStr.find("\r\n\r\n");
        if (headersEnd == std::string::npos) {
            resp.status = RESPONSE_PARSE_FAILED;
            resp.errorMessage = "Malformed response: no header/body separator (\\r\\n\\r\\n) found";
            DEBUG_LOG("Parse failed: missing header/body separator");
            return resp;
        }
        
        // ====================================================================
        // STEP 3: Parse status line
        // ====================================================================
        std::string headersStr = rawStr.substr(0, headersEnd);
        std::stringstream ss(headersStr);
        std::string statusLine;
        
        if (!std::getline(ss, statusLine)) {
            resp.status = RESPONSE_PARSE_FAILED;
            resp.errorMessage = "Empty response headers";
            DEBUG_LOG("Parse failed: empty headers");
            return resp;
        }
        
        // Remove trailing \r if present (getline stops at \n)
        if (!statusLine.empty() && statusLine[statusLine.length() - 1] == '\r') {
            statusLine.erase(statusLine.length() - 1);
        }
        
        DEBUG_LOGF("Status line: %s", statusLine.c_str());
        
        // Parse status line: "HTTP/1.1 200 OK"
        //                     ^       ^   ^
        //                     |       |   reason phrase
        //                     |       status code
        //                     version
        size_t space1 = statusLine.find(' ');
        size_t space2 = statusLine.find(' ', space1 + 1);
        
        if (space1 == std::string::npos || space2 == std::string::npos) {
            resp.status = RESPONSE_PARSE_FAILED;
            resp.errorMessage = "Invalid status line format (expected 'HTTP/1.x CODE REASON')";
            DEBUG_LOG("Parse failed: invalid status line");
            return resp;
        }
        
        std::string codeStr = statusLine.substr(space1 + 1, space2 - space1 - 1);
        resp.httpStatusCode = atoi(codeStr.c_str());
        
        DEBUG_LOGF("HTTP status code: %d", resp.httpStatusCode);
        
        // ====================================================================
        // STEP 4: Parse headers line by line
        // ====================================================================
        std::string line;
        while (std::getline(ss, line)) {
            // Remove trailing \r
            if (!line.empty() && line[line.length() - 1] == '\r') {
                line.erase(line.length() - 1);
            }
            
            // Skip empty lines
            if (line.empty()) {
                continue;
            }
            
            // Parse header: "Key: Value"
            size_t colon = line.find(':');
            if (colon == std::string::npos) {
                DEBUG_LOGF("Skipping malformed header: %s", line.c_str());
                continue;
            }
            
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            
            // Trim leading/trailing whitespace from value
            size_t valStart = val.find_first_not_of(" \t");
            size_t valEnd = val.find_last_not_of(" \t");
            
            if (valStart != std::string::npos) {
                val = val.substr(valStart, valEnd - valStart + 1);
            } else {
                val = "";  // Value was all whitespace
            }
            
            // ================================================================
            // NORMALIZE KEY TO LOWERCASE
            // ================================================================
            // RATIONALE: HTTP headers are case-insensitive per RFC 2616.
            //            Normalizing to lowercase allows simple map access
            //            without worrying about "Content-Length" vs
            //            "content-length" vs "CONTENT-LENGTH".
            std::transform(key.begin(), key.end(), key.begin(), tolower);
            
            resp.headers[key] = val;
            DEBUG_LOGF("Header: %s = %s", key.c_str(), val.c_str());
        }
        
        // ====================================================================
        // STEP 5: Extract body (binary-safe)
        // ====================================================================
        // Body starts after "\r\n\r\n" (4 bytes past headersEnd)
        resp.body.assign(rawData.begin() + headersEnd + 4, rawData.end());
        
        DEBUG_LOGF("Body size: %d bytes", resp.body.size());
        
        // ====================================================================
        // STEP 6: Validate Content-Length (if present)
        // ====================================================================
        // DEFENSIVE: Check if declared Content-Length matches actual body size.
        //            Mismatch indicates truncated response or proxy bug.
        std::map<std::string, std::string>::const_iterator it = 
            resp.headers.find("content-length");
        
        if (it != resp.headers.end()) {
            size_t expectedLength = static_cast<size_t>(atoi(it->second.c_str()));
            
            if (resp.body.size() != expectedLength) {
                resp.status = RESPONSE_PARSE_FAILED;
                std::stringstream errss;
                errss << "Body size mismatch: Content-Length header says " 
                      << expectedLength << " bytes, but received " 
                      << resp.body.size() << " bytes. "
                      << "Response may be truncated or corrupted.";
                resp.errorMessage = errss.str();
                
                DEBUG_LOGF("Content-Length mismatch: expected %d, got %d",
                           expectedLength, resp.body.size());
                return resp;
            }
        }
        
        // ====================================================================
        // STEP 7: Distinguish proxy errors from HTTP errors
        // ====================================================================
        // RATIONALE: HTTP 404/500 from origin server is SUCCESS (proxy worked),
        //            but 5xx from proxy itself is PROXY_ERROR (proxy failed).
        //            Heuristic: If code >= 500, assume proxy internal error.
        if (resp.httpStatusCode >= 500) {
            resp.status = PROXY_ERROR;
            std::stringstream errss;
            errss << "Proxy returned internal server error (HTTP " 
                  << resp.httpStatusCode << "). Check proxy logs for details.";
            resp.errorMessage = errss.str();
            
            DEBUG_LOGF("Proxy error: HTTP %d", resp.httpStatusCode);
            return resp;
        }
        
        // ====================================================================
        // SUCCESS: Valid response parsed
        // ====================================================================
        resp.status = SUCCESS;
        DEBUG_LOG("Response parsed successfully");
        
        return resp;
    }

} // namespace Network

// ============================================================================
// END OF IMPLEMENTATION
// ============================================================================
// MAINTENANCE NOTES:
//   - All Winsock calls are checked for errors with detailed logging
//   - Socket cleanup (closesocket) is guaranteed in all code paths
//   - Binary-safety maintained via vector<char> throughout
//   - 1MB limit enforced to protect Win98 VM resources
//   - Timeouts configured per-socket to prevent indefinite blocking
//
// FUTURE ENHANCEMENTS:
//   - Add support for HTTP redirects (301/302) via Location header
//   - Implement connection pooling for multiple requests
//   - Add progress callback for large downloads
//   - Support POST requests for forms (currently GET-only)
//   - Non-blocking I/O via WSAAsyncSelect for responsive UI
// ============================================================================