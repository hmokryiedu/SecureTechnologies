#pragma once
#include <windows.h>
#include <string>

template<typename T>
struct PerPipeStruct {
    OVERLAPPED overlap;
    HANDLE hPipe;
    char buffer[512];
    DWORD bytesToWrite;
    DWORD bytesRead;
    bool connected;
    int pipeId;
    T* userData;
    
    // Constructor — initializes fields and creates event for overlapped I/O
    PerPipeStruct() : hPipe(INVALID_HANDLE_VALUE), connected(false), 
                      bytesToWrite(0), bytesRead(0), pipeId(0), userData(nullptr) {
        ZeroMemory(&overlap, sizeof(OVERLAPPED));
        ZeroMemory(buffer, sizeof(buffer));
        overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    
    // Destructor — closes handles and releases memory
    ~PerPipeStruct() {
        if (overlap.hEvent) {
            CloseHandle(overlap.hEvent);
        }
        if (hPipe != INVALID_HANDLE_VALUE) {
            CloseHandle(hPipe);
        }
        if (userData) {
            delete userData;
        }
    }
    
    // Resets the structure for reuse (clears buffer and counters)
    void Reset() {
        ZeroMemory(buffer, sizeof(buffer));
        bytesRead = 0;
        bytesToWrite = 0;
        ResetEvent(overlap.hEvent);
    }
};
