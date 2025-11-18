// kernel32_stub.c - Stub for SetFilePointerEx on Win98
#include <windows.h>

// Stub implementation of SetFilePointerEx for Win98
BOOL WINAPI SetFilePointerEx(
    HANDLE hFile,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER lpNewFilePointer,
    DWORD dwMoveMethod
)
{
    LONG lDistanceHigh = liDistanceToMove.HighPart;
    DWORD dwResult = SetFilePointer(
        hFile,
        liDistanceToMove.LowPart,
        &lDistanceHigh,
        dwMoveMethod
    );
    
    if (dwResult == INVALID_SET_FILE_POINTER) {
        DWORD dwError = GetLastError();
        if (dwError != NO_ERROR) {
            return FALSE;
        }
    }
    
    if (lpNewFilePointer != NULL) {
        lpNewFilePointer->LowPart = dwResult;
        lpNewFilePointer->HighPart = lDistanceHigh;
    }
    
    return TRUE;
}
