#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include "PerPipeStruct.h"
#include "ServerConstants.h"

#define PIPE_NAME "\\\\.\\pipe\\AuthPipe" 
#define BUFFER_SIZE ServerConfig::PIPE_BUFFER_SIZE

using namespace std;

class PipeServer {
private:
    vector<HANDLE> threads;
    bool isRunning;
    HANDLE hShutdownEvent;  // Event for graceful thread shutdown
    UserList* userList;     // Store reference to unlock on stop

    // Fixed memory leak: Using SendMessage instead of PostMessage with dynamic allocation
    static void Log(HWND hGui, const string& text) {
        // SendMessage is synchronous, so we can use stack buffer safely
        SendMessage(hGui, WM_LOG_MSG, (WPARAM)text.c_str(), (LPARAM)text.length());
    }

    // АСИНХРОННИЙ (Overlapped) потік
    static DWORD WINAPI PipeInstanceThread(LPVOID lpvParam) {
        PerPipeData* data = (PerPipeData*)lpvParam;
        char buffer[BUFFER_SIZE];
        DWORD bytesTransferred;
        BOOL success;

        // 1. Ініціалізація Overlapped події (Manual Reset)
        data->GetOverlap()->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (data->GetOverlap()->hEvent == NULL) {
            Log(data->GetGui(), "Error creating event.");
            delete data;
            return 0;
        }

        // 2. Створюємо трубу з прапором FILE_FLAG_OVERLAPPED
        data->SetPipe(CreateNamedPipeA(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            BUFFER_SIZE, BUFFER_SIZE,
            0, NULL));

        if (data->GetPipe() == INVALID_HANDLE_VALUE) {
            Log(data->GetGui(), "Error creating pipe.");
            CloseHandle(data->GetOverlap()->hEvent);
            delete data;
            return 0;
        }

        // Main server loop with shutdown check
        while (WaitForSingleObject(data->GetShutdownEvent(), 0) == WAIT_TIMEOUT) {
            // 3. Асинхронне підключення (ConnectNamedPipe)
            ResetEvent(data->GetOverlap()->hEvent);
            
            BOOL connected = ConnectNamedPipe(data->GetPipe(), data->GetOverlap());

            if (connected) {
                 // Клієнт підключився миттєво
            }
            else {
                DWORD error = GetLastError();
                if (error == ERROR_IO_PENDING) {
                    if (!GetOverlappedResult(data->GetPipe(), data->GetOverlap(), &bytesTransferred, TRUE)) {
                        DisconnectNamedPipe(data->GetPipe());
                        continue; 
                    }
                }
                else if (error == ERROR_PIPE_CONNECTED) {
                    // Клієнт вже тут
                }
                else {
                    DisconnectNamedPipe(data->GetPipe());
                    continue;
                }
            }

            // --- КЛІЄНТ ПІДКЛЮЧЕНИЙ ---

            string loginStr, passStr;

            // 4. Асинхронне читання (ReadFile)
            ZeroMemory(buffer, BUFFER_SIZE);
            ResetEvent(data->GetOverlap()->hEvent);

            success = ReadFile(data->GetPipe(), buffer, BUFFER_SIZE - 1, &bytesTransferred, data->GetOverlap());

            if (!success && GetLastError() == ERROR_IO_PENDING) {
                success = GetOverlappedResult(data->GetPipe(), data->GetOverlap(), &bytesTransferred, TRUE);
            }

            if (success && bytesTransferred > 0) {
                // Buffer overflow protection
                if (bytesTransferred >= BUFFER_SIZE) {
                    bytesTransferred = BUFFER_SIZE - 1;
                }
                buffer[bytesTransferred] = '\0';
                
                string fullMessage(buffer);
                stringstream ss(fullMessage);
                ss >> loginStr;
                if (!ss.eof()) ss >> passStr;

                // Перевірка
                int res = data->GetUserList()->CheckUser(loginStr, passStr);
                DWORD replyValue = 0;

                if (res == -1) {
                    Log(data->GetGui(), "[PROTECT] Ignored: " + loginStr);
                    Sleep(ServerConfig::RETRY_DELAY_MS);
                    replyValue = 0;
                }
                else if (res == 1) {
                    Log(data->GetGui(), "[SUCCESS] Login: " + loginStr);
                    replyValue = 1;
                }
                else {
                    replyValue = 0;
                }

                // 5. Асинхронний запис (WriteFile)
                ResetEvent(data->GetOverlap()->hEvent);
                success = WriteFile(data->GetPipe(), &replyValue, sizeof(DWORD), &bytesTransferred, data->GetOverlap());
                
                if (!success && GetLastError() == ERROR_IO_PENDING) {
                    GetOverlappedResult(data->GetPipe(), data->GetOverlap(), &bytesTransferred, TRUE);
                }
                
                FlushFileBuffers(data->GetPipe());
            } else {
                // Enhanced error handling (Issue #10)
                DWORD error = GetLastError();
                
                if (error == ERROR_BROKEN_PIPE) {
                    Log(data->GetGui(), "[INFO] Client disconnected during read");
                } else if (error == ERROR_NO_DATA) {
                    Log(data->GetGui(), "[INFO] Pipe closing");
                } else if (bytesTransferred == 0 && success) {
                    // Graceful disconnect
                } else if (error != 0) {
                    char errorMsg[256];
                    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
                                  0, errorMsg, sizeof(errorMsg), NULL);
                    Log(data->GetGui(), "[ERROR] Pipe read error (" + to_string(error) + "): " + string(errorMsg));
                }
            }

            // 6. Відключення
            DisconnectNamedPipe(data->GetPipe());
        }

        CloseHandle(data->GetOverlap()->hEvent);
        CloseHandle(data->GetPipe());
        delete data;
        return 0;
    }

public:
    PipeServer() : isRunning(false), hShutdownEvent(NULL), userList(nullptr) {}

    void Start(int numPipes, UserList* uList, HWND hGui) {
        isRunning = true;
        userList = uList;
        userList->SetServerRunning(true);
        
        // Create shutdown event (manual-reset, initially non-signaled)
        hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        
        for (int i = 0; i < numPipes; i++) {
            PerPipeData* data = new PerPipeData(i + 1, hGui, uList, hShutdownEvent);
            
            HANDLE hThread = CreateThread(NULL, 0, PipeInstanceThread, data, 0, NULL);
            if (hThread) threads.push_back(hThread);
        }
        Log(hGui, "Async Server ON. Pipe: " + string(PIPE_NAME));
    }

    void Stop() {
        if (!isRunning) return;
        
        isRunning = false;
        
        if (hShutdownEvent) {
            // Signal all threads to exit
            SetEvent(hShutdownEvent);
            
            // Wait for all threads to finish
            if (threads.size() > 0) {
                DWORD result = WaitForMultipleObjects(
                    (DWORD)threads.size(),
                    threads.data(),
                    TRUE,  // Wait for all
                    ServerConfig::SHUTDOWN_TIMEOUT_MS
                );
                
                if (result == WAIT_TIMEOUT) {
                    // Log timeout but continue cleanup
                }
                
                // Close all thread handles
                for (HANDLE h : threads) {
                    CloseHandle(h);
                }
                threads.clear();
            }
            
            CloseHandle(hShutdownEvent);
            hShutdownEvent = NULL;
        }
        
        // Allow user list to be modified again
        if (userList) {
            userList->SetServerRunning(false);
            userList = nullptr;
        }
    }
    
    ~PipeServer() {
        Stop();
    }
};