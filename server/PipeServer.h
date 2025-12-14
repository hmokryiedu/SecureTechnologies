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
    HANDLE hShutdownEvent;  // Подія для коректного завершення
    UserList* userList;     // Вказівник на базу користувачів

    // Логування в GUI через SendMessage (безпечно для пам'яті)
    static void Log(HWND hGui, const string& text) {
        SendMessage(hGui, WM_LOG_MSG, (WPARAM)text.c_str(), (LPARAM)text.length());
    }

    // Робочий потік (Thread Function)
    static DWORD WINAPI PipeInstanceThread(LPVOID lpvParam) {
        PerPipeData* data = (PerPipeData*)lpvParam;
        char buffer[BUFFER_SIZE];
        DWORD bytesTransferred;
        BOOL success;

        // 1. Створення події для асинхронності
        data->GetOverlap()->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (data->GetOverlap()->hEvent == NULL) {
            Log(data->GetGui(), "Error creating event.");
            delete data;
            return 0;
        }

        // 2. Створення каналу (Pipe)
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

        // Головний цикл обробки клієнтів
        while (WaitForSingleObject(data->GetShutdownEvent(), 0) == WAIT_TIMEOUT) {
            // 3. Очікування підключення (Connect)
            ResetEvent(data->GetOverlap()->hEvent);
            BOOL connected = ConnectNamedPipe(data->GetPipe(), data->GetOverlap());

            if (!connected) {
                DWORD error = GetLastError();
                if (error == ERROR_IO_PENDING) {
                    // Чекаємо завершення асинхронної операції
                    if (!GetOverlappedResult(data->GetPipe(), data->GetOverlap(), &bytesTransferred, TRUE)) {
                        DisconnectNamedPipe(data->GetPipe());
                        continue; 
                    }
                } 
                else if (error == ERROR_PIPE_CONNECTED) {
                    // Клієнт підключився швидко
                }
                else {
                    DisconnectNamedPipe(data->GetPipe());
                    continue;
                }
            }

            // --- КЛІЄНТ ПІДКЛЮЧЕНИЙ ---

            string loginStr, passStr;

            // 4. Читання повідомлення (Read)
            ZeroMemory(buffer, BUFFER_SIZE);
            ResetEvent(data->GetOverlap()->hEvent);
            success = ReadFile(data->GetPipe(), buffer, BUFFER_SIZE - 1, &bytesTransferred, data->GetOverlap());

            if (!success && GetLastError() == ERROR_IO_PENDING) {
                success = GetOverlappedResult(data->GetPipe(), data->GetOverlap(), &bytesTransferred, TRUE);
            }

            if (success && bytesTransferred > 0) {
                // Обробка буфера
                if (bytesTransferred >= BUFFER_SIZE) bytesTransferred = BUFFER_SIZE - 1;
                buffer[bytesTransferred] = '\0';
                
                string fullMessage(buffer);
                stringstream ss(fullMessage);
                ss >> loginStr;
                if (!ss.eof()) ss >> passStr;

                // === ПЕРЕВІРКА КОРИСТУВАЧА ===
                bool needDelay = false;
                
                // Викликаємо функцію, яка повертає результат і чи потрібна затримка
                int checkResult = data->GetUserList()->CheckUser(loginStr, passStr, needDelay);
                
                // 1. Якщо треба затримка (бо було > 3 помилок) — чекаємо
                if (needDelay) {
                    Log(data->GetGui(), "[THROTTLE] Too many attempts (" + loginStr + "). Delaying 1s...");
                    Sleep(ServerConfig::THROTTLE_DELAY_MS); // Спимо 1000 мс (1 сек)
                }

                // 2. Логуємо результат
                if (checkResult == 1) {
                    Log(data->GetGui(), "[SUCCESS] Login: " + loginStr);
                } else {
                    Log(data->GetGui(), "[FAIL] Bad password: " + loginStr);
                }

                // 3. Відправляємо відповідь (ЗАВЖДИ)
                // Ми відсилаємо реальний результат (1 або 0) незалежно від того, спали ми чи ні.
                DWORD replyValue = (DWORD)checkResult;

                ResetEvent(data->GetOverlap()->hEvent);
                success = WriteFile(data->GetPipe(), &replyValue, sizeof(DWORD), &bytesTransferred, data->GetOverlap());
                
                if (!success && GetLastError() == ERROR_IO_PENDING) {
                    GetOverlappedResult(data->GetPipe(), data->GetOverlap(), &bytesTransferred, TRUE);
                }
                
                FlushFileBuffers(data->GetPipe());
            } else {
                // Обробка помилок читання
                DWORD error = GetLastError();
                if (error != ERROR_BROKEN_PIPE && error != ERROR_NO_DATA && error != 0) {
                     // Log error if needed
                }
            }

            // 6. Відключення клієнта (Disconnect)
            // Ми завжди відключаємо клієнта після відповіді, щоб він перепідключався
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
        
        // Подія для зупинки
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
            // Сигнал зупинки
            SetEvent(hShutdownEvent);
            
            // Чекаємо завершення потоків
            if (threads.size() > 0) {
                WaitForMultipleObjects((DWORD)threads.size(), threads.data(), TRUE, ServerConfig::SHUTDOWN_TIMEOUT_MS);
                
                for (HANDLE h : threads) CloseHandle(h);
                threads.clear();
            }
            
            CloseHandle(hShutdownEvent);
            hShutdownEvent = NULL;
        }
        
        if (userList) {
            userList->SetServerRunning(false);
            userList = nullptr;
        }
    }
    
    ~PipeServer() {
        Stop();
    }
};