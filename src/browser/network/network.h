// ============================================================================
// network.h - Public Interface for the Network Module
// ============================================================================
// MODULE PURPOSE:
// Provides a high-level, blocking, and binary-safe abstraction layer over the
// Winsock 2.2 API for the Win98 Retro Browser. This module's primary 
// responsibility is to handle ALL network communication by forwarding HTTP 
// GET requests to a local Python proxy, which in turn handles modern 
// HTTPS/TLS/HTTP2 and web standards that Winsock 2.2 cannot support.
//
// ARCHITECTURE OVERVIEW:
//
//   +-----------------------+       +-------------------+       +--------------+
//   | Win98 Browser (C++)   | ----> | Python Proxy      | ----> | Modern Web   |
//   | [Winsock 2.2 / HTTP]  |       | [Requests/HTTPS]  |       | [HTTPS/HTTP2]|
//   +-----------------------+       +-------------------+       +--------------+
//         ^                               ^                           |
//         |                               |                           |
//         +---------- This Module --------+                           |
//                                                                     |
//   Response Path: <--------------------------------------------<-----+
//
// DESIGN PHILOSOPHY:
// 1.  COMPLETE ABSTRACTION: The caller (e.g., UI module) is fully shielded
//     from socket complexities, protocol details, and proxy mechanisms. The
//     interface exposes only URLs (strings) and structured responses (structs).
// 2.  RAII (Resource Acquisition Is Initialization): The NetworkManager class
//     manages Winsock's lifecycle. Constructor calls WSAStartup(); destructor
//     calls WSACleanup(). This guarantees no resource leaks even during
//     exceptions or early exits. Winsock is initialized exactly ONCE.
// 3.  BLOCKING SYNCHRONOUS MODEL: All operations are blocking. FetchUrl() will
//     NOT return until the request completes, errors out, or times out. This
//     design simplifies integration into a single-threaded Win32 message loop.
// 4.  BINARY-SAFETY: Response bodies use std::vector<char>, correctly handling
//     binary content (images, PDFs) that may contain embedded null terminators.
//     No premature string truncation occurs.
// 5.  ROBUST ERROR HANDLING: Clear enum (NetworkStatus) + comprehensive struct
//     (HttpResponse) provide detailed error information. Callers can react
//     appropriately to each failure mode (proxy down, timeout, DNS failure).
// 6.  DEFENSIVE PARSING: All proxy responses are validated before use. Malformed
//     headers or bodies trigger RESPONSE_PARSE_FAILED, preventing crashes.
// 7.  EXPLICIT RESOURCE LIMITS: 1MB response body cap prevents memory exhaustion
//     attacks or proxy bugs from crashing the Win98 VM (limited RAM).
//
// USAGE PATTERN (Production-Ready Example):
//     ```cpp
//     #include "network.h"
//     
//     Network::NetworkManager netMgr("127.0.0.1", 8080);
//     if (!netMgr.IsInitialized()) {
//         MessageBox(NULL, "Winsock init failed! Check OS network stack.", 
//                    "Fatal Error", MB_ICONERROR);
//         return 1;  // Cannot proceed without networking
//     }
//     
//     SetCursor(LoadCursor(NULL, IDC_WAIT));  // Show hourglass (blocking call!)
//     Network::HttpResponse resp = netMgr.FetchUrl("http://example.com");
//     SetCursor(LoadCursor(NULL, IDC_ARROW)); // Restore normal cursor
//     
//     if (resp.status != Network::SUCCESS) {
//         MessageBox(NULL, resp.errorMessage.c_str(), "Network Error", MB_ICONWARNING);
//         return;
//     }
//     
//     if (resp.httpStatusCode == 200) {
//         // For HTML: Convert binary body to string
//         std::string html(resp.body.begin(), resp.body.end());
//         // ... parse and render HTML ...
//     } else if (resp.httpStatusCode == 404) {
//         ShowErrorPage("Page Not Found");
//     }
//     ```
//
// TESTING RECOMMENDATIONS:
//     UNIT TESTS:
//       - Valid URL (http://example.com) -> Verify status=SUCCESS, code=200
//       - Invalid URL schemes (ftp://, javascript:) -> INVALID_URL
//       - Overly long URL (>2048 chars) -> INVALID_URL
//       - Proxy offline -> PROXY_CONNECT_FAILED within timeout window
//       - Proxy returns HTTP 404/500 -> status=SUCCESS, httpStatusCode=404/500
//       - Large response (near 1MB) -> Verify full body received
//       - Binary image download -> Verify no null-terminator corruption
//     INTEGRATION TESTS:
//       - Browser -> Proxy -> Real website -> Verify end-to-end rendering
//       - Proxy crash mid-response -> Verify graceful RECV_FAILED error
//     STRESS TESTS:
//       - Rapid repeated requests (memory leak check with Task Manager)
//       - Concurrent NetworkManager instances (should work if each on own thread)
//
// THREAD SAFETY:
//     NOT THREAD-SAFE. Each NetworkManager instance is designed for single-
//     threaded use within one Win32 message pump. Do NOT call FetchUrl() from
//     multiple threads simultaneously. If multi-threading is required, create
//     one NetworkManager per thread (Winsock allows this pattern).
//
// DEPENDENCIES:
//     - Winsock 2.2 library (ws2_32.lib): Must be linked in project settings.
//     - Python 3.x proxy server: Must be running on configured host:port before
//       any FetchUrl() calls. Proxy failure causes PROXY_CONNECT_FAILED.
//     - stdafx.h: Provides all system headers (windows.h, winsock2.h, STL).
// ============================================================================

#pragma once

#ifndef NETWORK_H
#define NETWORK_H

#include "stdafx.h" // The ONLY include. Contains all necessary system/STL headers.

// ============================================================================
//  NAMESPACE: Network
//  Encapsulates all networking functionality to prevent global scope pollution.
// ============================================================================
namespace Network
{
    /**
     * @enum NetworkStatus
     * @brief Defines the possible outcomes of a network request operation.
     *        This should be the FIRST value checked in an HttpResponse before
     *        accessing any other fields. Only when status==SUCCESS are 
     *        httpStatusCode, headers, and body guaranteed to be valid.
     */
    enum NetworkStatus
    {
        /// @brief Request completed successfully. Check httpStatusCode for HTTP-level status (200, 404, etc.).
        ///        This does NOT mean the HTTP status is 200, only that the network operation succeeded.
        SUCCESS,

        /// @brief Failed to initialize Winsock 2.2 library via WSAStartup().
        ///        CAUSES: OS network stack corrupted, ws2_32.dll missing, or version mismatch.
        ///        USER ACTION: Reinstall Windows networking components or check Control Panel > Network.
        WINSOCK_INIT_FAILED,

        /// @brief Could not establish TCP connection to the local proxy server.
        ///        CAUSES: (1) Proxy not running, (2) Firewall blocking port 8080, 
        ///                (3) VM network adapter misconfigured (NAT vs Bridged).
        ///        USER ACTION: Verify proxy is running via `python proxy.py`, check VM network 
        ///                     adapter settings, ensure localhost (127.0.0.1) is reachable.
        PROXY_CONNECT_FAILED,

        /// @brief The provided URL was malformed or exceeded internal length limits.
        ///        CAUSES: (1) Missing "http://" scheme, (2) URL longer than 2048 characters,
        ///                (3) Invalid characters or encoding.
        ///        USER ACTION: Validate URL format before passing to FetchUrl().
        INVALID_URL,

        /// @brief Error occurred while sending the HTTP request to the proxy via send().
        ///        CAUSES: (1) Proxy closed connection prematurely, (2) Network cable unplugged,
        ///                (3) VM network suspended, (4) Winsock internal error.
        ///        USER ACTION: Check network connectivity and retry the request.
        SEND_FAILED,

        /// @brief Error occurred while receiving the response from the proxy via recv().
        ///        CAUSES: (1) Proxy crashed mid-response, (2) Network disconnected during download,
        ///                (3) Response exceeded 1MB limit (safety cap), (4) Socket closed unexpectedly.
        ///        USER ACTION: Restart proxy, check network stability, or reduce content size.
        RECV_FAILED,

        /// @brief The connection/request timed out waiting for a response.
        ///        CAUSES: (1) Proxy is hung or slow, (2) Remote web server not responding,
        ///                (3) Network latency too high, (4) DNS resolution stalled.
        ///        USER ACTION: Increase timeout value or check proxy logs for slow upstream requests.
        TIMEOUT,

        /// @brief The proxy successfully connected but returned a non-200 status, indicating an error.
        ///        CAUSES: Proxy encountered an internal error processing the request.
        ///        USER ACTION: Check proxy logs (stdout/stderr) for Python traceback or error messages.
        ///        NOTE: This is different from HTTP 404/500, which show as SUCCESS with httpStatusCode=404.
        PROXY_ERROR,

        /// @brief The response from the proxy was malformed and could not be parsed.
        ///        CAUSES: (1) Proxy sent invalid HTTP headers, (2) Missing Content-Length,
        ///                (3) Corrupted data during transmission, (4) Proxy implementation bug.
        ///        USER ACTION: Update proxy to latest version or file a bug report with raw response.
        RESPONSE_PARSE_FAILED,

        /// @brief An unexpected or unhandled error occurred (should never happen in production).
        ///        CAUSES: Unhandled exception, logic error, or missing error case in code.
        ///        USER ACTION: File a bug report with steps to reproduce. This indicates a code defect.
        UNKNOWN_ERROR
    };

    /**
     * @struct HttpResponse
     * @brief A self-contained, immutable-after-return structure holding the complete
     *        result of a network request. Designed for value semantics (safe to copy).
     * 
     * USAGE RULES:
     *   1. ALWAYS check `status` field first before accessing other fields.
     *   2. If status != SUCCESS, only `errorMessage` is valid. Other fields are undefined.
     *   3. If status == SUCCESS, all fields are valid and safe to use.
     */
    struct HttpResponse
    {
        /// @brief The overall operation status. CHECK THIS FIRST.
        ///        Only when this equals SUCCESS are the other fields meaningful.
        NetworkStatus status;

        /// @brief Standard HTTP status code returned by the origin server (e.g., 200, 404, 500).
        ///        VALID ONLY IF: status == SUCCESS.
        ///        RANGE: Typically 100-599 per HTTP spec.
        ///        EXAMPLES: 200 (OK), 301 (Redirect), 404 (Not Found), 500 (Server Error).
        int httpStatusCode;

        /// @brief Map of HTTP response headers. Keys are NORMALIZED to lowercase for case-insensitive lookup.
        ///        VALID ONLY IF: status == SUCCESS.
        ///        EXAMPLES: headers["content-type"] -> "text/html; charset=utf-8"
        ///                  headers["content-length"] -> "4567"
        ///        NOTE: Use lowercase keys when accessing (e.g., "content-type", not "Content-Type").
        std::map<std::string, std::string> headers;

        /// @brief The response body as a raw byte vector. BINARY-SAFE (handles embedded nulls).
        ///        VALID ONLY IF: status == SUCCESS.
        ///        SIZE LIMIT: Maximum 1MB (1,048,576 bytes) enforced to prevent Win98 VM memory exhaustion.
        ///        USAGE FOR TEXT (HTML, JSON, XML):
        ///            std::string text(body.begin(), body.end());
        ///        USAGE FOR BINARY (Images, PDFs):
        ///            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, body.size());
        ///            void* pMem = GlobalLock(hMem);
        ///            memcpy(pMem, &body[0], body.size());
        ///            GlobalUnlock(hMem);
        ///        WARNING: Do NOT assume body is null-terminated. Use body.size() for length.
        std::vector<char> body;

        /// @brief Human-readable error message providing context for failures.
        ///        BEHAVIOR:
        ///          - If status == SUCCESS: Contains empty string or non-critical warnings.
        ///          - If status != SUCCESS: Contains detailed error description including:
        ///              * Winsock error codes (e.g., "connect() failed: WSAECONNREFUSED (10061)")
        ///              * Diagnostic hints (e.g., "Verify proxy is running on 127.0.0.1:8080")
        ///              * Parser errors (e.g., "Missing Content-Length header in response")
        ///        USAGE: Display to user in MessageBox or log to file for debugging.
        std::string errorMessage;

        /**
         * @brief Default constructor initializing to a safe, sentinel error state.
         *        This ensures uninitialized responses are immediately detectable as invalid.
         */
        HttpResponse() 
            : status(UNKNOWN_ERROR), 
              httpStatusCode(0), 
              errorMessage("Uninitialized response object - FetchUrl() was never called") 
        {}
    };

    /**
     * @class NetworkManager
     * @brief Manages the network session lifecycle and provides methods for fetching web content.
     *        Employs the RAII (Resource Acquisition Is Initialization) pattern to handle 
     *        Winsock initialization and cleanup automatically.
     *
     * RAII GUARANTEES:
     *   - Constructor calls WSAStartup() exactly ONCE. If it fails, IsInitialized() returns false.
     *   - Destructor calls WSACleanup() exactly ONCE, even if exceptions occur or program exits early.
     *   - No manual cleanup required. No risk of resource leaks or double-cleanup.
     *
     * USAGE CONSTRAINTS:
     *   - NOT THREAD-SAFE: Each instance is designed for single-threaded use within one message loop.
     *     Do NOT call methods from multiple threads concurrently on the same instance.
     *   - If multi-threading is required, create ONE NetworkManager per thread (Winsock supports this).
     *   - Typically instantiate ONCE at application startup as a global or long-lived local variable.
     *
     * PERFORMANCE NOTES:
     *   - Each FetchUrl() call opens a new TCP socket, sends request, receives response, then closes socket.
     *   - No connection pooling or keep-alive (HTTP/1.0 semantics). This is intentional for simplicity.
     *   - For multiple requests, consider a small delay between calls to avoid proxy overload.
     *
     * @note Designed for singleton or near-singleton usage pattern (one per app or one per thread).
     */
    class NetworkManager
    {
    public:
        /**
         * @brief Constructs the NetworkManager and initializes the Winsock 2.2 library.
         *        This is where WSAStartup() is called. Check IsInitialized() after construction.
         * 
         * @param proxyHost The hostname or IP address of the proxy server (default: 127.0.0.1).
         * @param proxyPort The TCP port number of the proxy server (default: 8080).
         * 
         * NOTES:
         *   - Constructor is marked `explicit` to prevent accidental implicit type conversions.
         *   - If WSAStartup() fails, constructor completes but IsInitialized() will return false.
         *   - The proxy configuration can be changed later via SetProxy() if needed.
         * 
         * EXAMPLE:
         *     Network::NetworkManager netMgr;  // Uses defaults (127.0.0.1:8080)
         *     if (!netMgr.IsInitialized()) {
         *         MessageBox(NULL, "Network init failed!", "Error", MB_ICONERROR);
         *         return 1;
         *     }
         */
        explicit NetworkManager(const std::string& proxyHost = PROXY_DEFAULT_HOST, int proxyPort = PROXY_DEFAULT_PORT);

        /**
         * @brief Destroys the NetworkManager and cleans up the Winsock library.
         */
        ~NetworkManager();

        /**
         * @brief Checks if the Winsock library was successfully initialized.
         * @return true if initialized and ready for network operations, false otherwise.
         */
        bool IsInitialized() const;

        /**
         * @brief Updates the proxy server configuration.
         * @param host The new hostname or IP address of the proxy.
         * @param port The new port number for the proxy.
         */
        void SetProxy(const std::string& host, int port);

        /**
         * @brief Fetches content from a given URL in a BLOCKING manner. This is the core
         *        method of the network module, performing a complete request-response cycle.
         *
         * OPERATION SEQUENCE (7 Steps):
         *   1. INPUT VALIDATION: Checks URL format (must start with "http://"), length (<2048 chars).
         *      If invalid, returns immediately with status=INVALID_URL.
         *   
         *   2. SOCKET CREATION: Creates a new TCP socket via socket(AF_INET, SOCK_STREAM, IPPROTO_TCP).
         *      Each call gets a fresh socket (no connection reuse).
         *   
         *   3. PROXY CONNECTION: Establishes TCP connection to configured proxy (default 127.0.0.1:8080).
         *      Uses blocking connect() with built-in timeout. If proxy is unreachable, returns
         *      status=PROXY_CONNECT_FAILED within ~20 seconds (Winsock default).
         *   
         *   4. REQUEST CONSTRUCTION: Builds a minimal but valid HTTP/1.1 GET request:
         *          GET <url> HTTP/1.1\r\n
         *          Host: <proxy-host>\r\n
         *          Connection: close\r\n
         *          \r\n
         *   
         *   5. REQUEST TRANSMISSION: Sends the request via send(). If proxy closes connection
         *      prematurely, returns status=SEND_FAILED.
         *   
         *   6. RESPONSE RECEPTION: Calls recv() in a loop until:
         *      - Socket closes (recv returns 0), OR
         *      - 1MB size limit reached (returns status=RECV_FAILED with warning), OR
         *      - Socket error occurs (returns status=RECV_FAILED with WSA error code).
         *      Data is accumulated into a std::vector<char> buffer.
         *   
         *   7. RESPONSE PARSING: Parses raw buffer into HttpResponse struct:
         *      - Extracts status line (e.g., "HTTP/1.1 200 OK")
         *      - Parses headers line-by-line (keys normalized to lowercase)
         *      - Separates body from headers at "\r\n\r\n" boundary
         *      - Validates Content-Length if present
         *      If parsing fails, returns status=RESPONSE_PARSE_FAILED.
         *
         * @param url The ABSOLUTE URL to fetch. MUST start with "http://" (not "https://").
         *            Max length: 2048 characters (enforced for safety).
         *            Examples: "http://example.com", "http://192.168.1.1/page.html"
         * 
         * @return HttpResponse struct containing the result. ALWAYS check response.status FIRST:
         *         - If SUCCESS: response.httpStatusCode, headers, and body are valid.
         *         - If not SUCCESS: Only response.errorMessage is valid (contains diagnostic info).
         *
         * CRITICAL WARNING - BLOCKING CALL:
         *   This function is SYNCHRONOUS and will FREEZE the calling thread for the duration
         *   of the request (typically 100ms - 5 seconds, but up to 20s on timeout). On Win98's
         *   single-threaded message loop, this means:
         *     - The UI will become UNRESPONSIVE (no button clicks, no repaints)
         *     - The window may show "(Not Responding)" in the title bar
         *   
         *   MITIGATION: Always call SetCursor(LoadCursor(NULL, IDC_WAIT)) BEFORE FetchUrl()
         *               and SetCursor(LoadCursor(NULL, IDC_ARROW)) AFTER to show user feedback.
         *   
         *   FUTURE IMPROVEMENT: For non-blocking behavior, run FetchUrl() in a background thread
         *                       and use PostMessage() to notify the UI thread when complete.
         *
         * @test UNIT TESTS MUST COVER:
         *       - Valid URL (http://example.com) -> status=SUCCESS, httpStatusCode=200
         *       - Invalid URL scheme (ftp://site.com, https://site.com) -> status=INVALID_URL
         *       - Overly long URL (>2048 chars) -> status=INVALID_URL
         *       - Proxy offline (manually stop proxy.py) -> status=PROXY_CONNECT_FAILED
         *       - Proxy returns HTTP 404 -> status=SUCCESS, httpStatusCode=404
         *       - Proxy returns HTTP 500 -> status=SUCCESS, httpStatusCode=500
         *       - Large response (e.g., 500KB HTML) -> status=SUCCESS, body.size() == expected
         *       - Binary image download (e.g., PNG) -> status=SUCCESS, body contains valid PNG header
         *       - Malformed proxy response (missing headers) -> status=RESPONSE_PARSE_FAILED
         */
        HttpResponse FetchUrl(const std::string& url) const;

    private:
        // ====================================================================
        // NON-COPYABLE DESIGN (C++03 Pattern)
        // ====================================================================
        // Disable copy constructor and assignment operator to prevent accidental 
        // duplication of a manager that controls a global resource (Winsock).
        // RATIONALE: Copying would lead to double WSACleanup() calls, corrupting
        //            the Winsock reference count and crashing the application.
        // NOTE: In C++11+, we would use "= delete" syntax instead.
        NetworkManager(const NetworkManager&);
        NetworkManager& operator=(const NetworkManager&);

        // ====================================================================
        // MEMBER VARIABLES
        // ====================================================================
        bool m_initialized;         ///< True if WSAStartup() succeeded, false otherwise.
        std::string m_proxyHost;    ///< Hostname or IP address of the proxy server (e.g., "127.0.0.1").
        int m_proxyPort;            ///< TCP port number of the proxy server (e.g., 8080).

        // ====================================================================
        // PRIVATE HELPER METHODS (Implementation Details)
        // ====================================================================
        // These methods are declared here for transparency and maintainability,
        // allowing readers of the header to understand the high-level implementation
        // structure without reading the .cpp file. They remain private to preserve
        // encapsulation and prevent external code from calling them directly.

        /**
         * @brief Constructs a minimal HTTP/1.1 GET request string for the given URL.
         * @param url The target URL (e.g., "http://example.com/page.html").
         * @return A properly formatted HTTP request string with \r\n line endings.
         */
        std::string BuildRequest(const std::string& url) const;

        /**
         * @brief Parses raw HTTP response bytes into an HttpResponse struct.
         * @param rawData The complete raw response received from the proxy (headers + body).
         * @return Parsed HttpResponse with status, headers, and body separated.
         *         If parsing fails, returns response with status=RESPONSE_PARSE_FAILED.
         */
        HttpResponse ParseResponse(const std::vector<char>& rawData) const;

        /**
         * @brief Establishes a TCP connection to the configured proxy server.
         * @return A connected SOCKET handle on success, or INVALID_SOCKET on failure.
         *         Caller is responsible for calling closesocket() on the returned socket.
         */
        SOCKET Connect() const;

        /**
         * @brief Receives all available data from a socket until it closes or an error occurs.
         * @param sock The connected socket to read from.
         * @param data Output parameter; received bytes are appended to this vector.
         * @note Enforces 1MB size limit to prevent memory exhaustion. If exceeded, stops
         *       receiving and the caller should return RECV_FAILED status.
         */
        void ReceiveAll(SOCKET sock, std::vector<char>& data) const;
    };

} // namespace Network

#endif // NETWORK_H