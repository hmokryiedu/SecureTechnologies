#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include "PerPipeStruct.h"

#define PIPE_NAME "\\\\.\\pipe\\AuthPipe" 
#define BUFFER_SIZE 512

class PipeServer {
private:
    vector<HANDLE> threads;
    bool isRunning;

    // Логування в GUI
    static void Log(HWND hGui, const string& text) {
        char* buf = new char[text.length() + 1];
        strcpy_s(buf, text.length() + 1, text.c_str());
        PostMessage(hGui, WM_LOG_MSG, (WPARAM)buf, 0);
    }

    // Потік обробки
    static DWORD WINAPI PipeInstanceThread(LPVOID lpvParam) {
        PerPipeData* data = (PerPipeData*)lpvParam;
        char buffer[BUFFER_SIZE];
        DWORD bytesRead, bytesWritten;
        BOOL success;

        while (true) {
            // 1. Створюємо канал
            data->hPipe = CreateNamedPipeA(
                PIPE_NAME,
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                BUFFER_SIZE, BUFFER_SIZE,
                0, NULL);

            if (data->hPipe == INVALID_HANDLE_VALUE) {
                Log(data->hGui, "Error creating pipe.");
                break;
            }

            // 2. Чекаємо клієнта
            BOOL connected = ConnectNamedPipe(data->hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

            if (connected) {
                string loginStr, passStr;

                // 3. Читаємо повідомлення (Логін + Пароль)
                ZeroMemory(buffer, BUFFER_SIZE);
                success = ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL);

                if (success && bytesRead > 0) {
                    string fullMessage(buffer);
                    stringstream ss(fullMessage);
                    ss >> loginStr;
                    if (!ss.eof()) ss >> passStr;

                    // 4. Перевірка
                    int res = data->userList->CheckUser(loginStr, passStr);
                    DWORD replyValue = 0; 

                    if (res == -1) {
                        // Блокування логуємо
                        Log(data->hGui, "[PROTECT] Ignored: " + loginStr);
                        Sleep(1000);
                        replyValue = 0;
                    }
                    else if (res == 1) {
                        // Успіх логуємо
                        Log(data->hGui, "[SUCCESS] Login: " + loginStr);
                    }
                    else {
                        // Невдачу не логуємо
                        replyValue = 0;
                    }

                    // 5. Відправка відповіді (4 байти числа)
                    WriteFile(data->hPipe, &replyValue, sizeof(DWORD), &bytesWritten, NULL);
                    
                    // Примусовий запис і невелика пауза
                    FlushFileBuffers(data->hPipe);    
                }

                // 6. Розрив з'єднання
                DisconnectNamedPipe(data->hPipe);
            }
            CloseHandle(data->hPipe);
        }

        delete data;
        return 0;
    }

public:
    PipeServer() : isRunning(false) {}

    void Start(int numPipes, UserList* uList, HWND hGui) {
        isRunning = true;
        for (int i = 0; i < numPipes; i++) {
            PerPipeData* data = new PerPipeData;
            data->pipeId = i + 1;
            data->userList = uList;
            data->hGui = hGui;
            
            HANDLE hThread = CreateThread(NULL, 0, PipeInstanceThread, data, 0, NULL);
            if (hThread) threads.push_back(hThread);
        }
        Log(hGui, "Server ON. Pipe: " + string(PIPE_NAME));
    }
};