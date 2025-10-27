#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "PerPipeStruct.h"
#include "list.h"

template<typename T>
class PipeServer {
private:
    std::string pipeName;
    int numInstances;
    List<PerPipeStruct<T>*> pipeList;
    bool isRunning;
    HANDLE hStopEvent;
    
    typedef void (*ReadCallback)(PerPipeStruct<T>*, void*);
    typedef void (*ConnectCallback)(PerPipeStruct<T>*, void*);
    typedef void (*DisconnectCallback)(PerPipeStruct<T>*, void*);
    
    ReadCallback onReadCallback;
    ConnectCallback onConnectCallback;
    DisconnectCallback onDisconnectCallback;
    void* callbackUserData;

public:
    PipeServer(const std::string& name, int instances = 3) 
        : pipeName(name), numInstances(instances), isRunning(false),
          onReadCallback(nullptr), onConnectCallback(nullptr), 
          onDisconnectCallback(nullptr), callbackUserData(nullptr) {
        hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    
    ~PipeServer() {
        Stop();
        CloseHandle(hStopEvent);
        
        // Cleanup of all pipes
        for (auto it = pipeList.begin(); it != pipeList.end(); ++it) {
            delete *it;
        }
        pipeList.clear();
    }
    
    void SetReadCallback(ReadCallback callback, void* userData = nullptr) {
        onReadCallback = callback;
        callbackUserData = userData;
    }
    
    void SetConnectCallback(ConnectCallback callback, void* userData = nullptr) {
        onConnectCallback = callback;
        callbackUserData = userData;
    }
    
    void SetDisconnectCallback(DisconnectCallback callback, void* userData = nullptr) {
        onDisconnectCallback = callback;
        callbackUserData = userData;
    }
    
    bool Start() {
        if (isRunning) return false;
        
        ResetEvent(hStopEvent);
        isRunning = true;
        
        // Creating pipe instances
        for (int i = 0; i < numInstances; i++) {
            PerPipeStruct<T>* pipeStruct = new PerPipeStruct<T>();
            pipeStruct->pipeId = i + 1;
            
            pipeStruct->hPipe = CreateNamedPipeA(
                pipeName.c_str(),
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                512, 512, 0, NULL
            );
            
            if (pipeStruct->hPipe == INVALID_HANDLE_VALUE) {
                delete pipeStruct;
                continue;
            }
            
            pipeList.push_back(pipeStruct);
            
            // Asynchronous connection waiting
            ConnectNamedPipe(pipeStruct->hPipe, &pipeStruct->overlap);
        }
        
        return true;
    }
    
    void Stop() {
        if (!isRunning) return;
        
        isRunning = false;
        SetEvent(hStopEvent);
        
        // Closing all pipes
        for (auto it = pipeList.begin(); it != pipeList.end(); ++it) {
            PerPipeStruct<T>* ps = *it;
            if (ps->connected) {
                DisconnectNamedPipe(ps->hPipe);
            }
            CancelIo(ps->hPipe);
        }
    }
    
    void ProcessEvents(DWORD timeout = 100) {
        if (!isRunning) return;
        
        std::vector<HANDLE> events;
        std::vector<PerPipeStruct<T>*> eventPipes;
        
        events.push_back(hStopEvent);
        
        // Collecting all events
        for (auto it = pipeList.begin(); it != pipeList.end(); ++it) {
            PerPipeStruct<T>* ps = *it;
            events.push_back(ps->overlap.hEvent);
            eventPipes.push_back(ps);
        }
        
        DWORD result = WaitForMultipleObjects(events.size(), events.data(), 
                                              FALSE, timeout);
        
        if (result == WAIT_OBJECT_0) {
            // Stop event
            return;
        }
        
        if (result > WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + events.size()) {
            int index = result - WAIT_OBJECT_0 - 1;
            PerPipeStruct<T>* ps = eventPipes[index];
            
            DWORD bytesTransferred;
            BOOL success = GetOverlappedResult(ps->hPipe, &ps->overlap, 
                                               &bytesTransferred, FALSE);
            
            if (!ps->connected) {
                // Client connected
                ps->connected = true;
                if (onConnectCallback) {
                    onConnectCallback(ps, callbackUserData);
                }
                
                // Start reading data
                ps->Reset();
                ReadFile(ps->hPipe, ps->buffer, sizeof(ps->buffer) - 1, 
                        NULL, &ps->overlap);
                
            } else if (success && bytesTransferred > 0) {
                // Data received
                ps->buffer[bytesTransferred] = '\0';
                ps->bytesRead = bytesTransferred;
                
                if (onReadCallback) {
                    onReadCallback(ps, callbackUserData);
                }
                
            } else {
                // Client disconnected or error
                if (onDisconnectCallback) {
                    onDisconnectCallback(ps, callbackUserData);
                }
                
                DisconnectNamedPipe(ps->hPipe);
                ps->connected = false;
                ps->Reset();
                
                // Waiting for new connection
                ConnectNamedPipe(ps->hPipe, &ps->overlap);
            }
        }
    }
    
    void WriteResponse(PerPipeStruct<T>* ps, const char* data, DWORD size) {
        DWORD bytesWritten;
        WriteFile(ps->hPipe, data, size, &bytesWritten, NULL);
        FlushFileBuffers(ps->hPipe);
    }
    
    bool IsRunning() const { return isRunning; }
    
    int GetPipeCount() const { return pipeList.getSize(); }
};