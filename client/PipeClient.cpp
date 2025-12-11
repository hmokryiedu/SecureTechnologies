#include "PipeClient.h"
#include <iostream>
#include <sstream>

PipeClient::PipeClient(const std::string& name)
    : hPipe(INVALID_HANDLE_VALUE), pipeName(name) {
}

PipeClient::~PipeClient() {
    Disconnect();
}

bool PipeClient::Connect(const std::string& computerName) {
    std::string fullPipeName = "\\\\.\\pipe\\AuthPipe";
    
    // Таймер для захисту від вічного зависання (макс 10 секунд спроб)
    DWORD startTime = GetTickCount();

    while ((GetTickCount() - startTime) < 10000) {
        // Спроба відкрити файл (підключитися до труби)
        hPipe = CreateFileA(fullPipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

        // Якщо успішно — виходимо
        if (hPipe != INVALID_HANDLE_VALUE) {
            return true;
        }

        DWORD error = ::GetLastError();

        // СИТУАЦІЯ 1: Пайп існує, але зайнятий іншим клієнтом
        if (error == ERROR_PIPE_BUSY) {
            // Чекаємо звільнення (до 1 сек)
            if (!WaitNamedPipeA(fullPipeName.c_str(), 1000)) {
                continue; // Тайм-аут очікування, пробуємо цикл заново
            }
            continue; // Пайп звільнився, пробуємо CreateFile знову
        }

        // СИТУАЦІЯ 2: Пайпа ще немає (Сервер перезапускає його) - ЦЕ ВАША ПОМИЛКА
        if (error == ERROR_FILE_NOT_FOUND) {
            Sleep(10); // Чекаємо 10 мс, даємо серверу час створити трубу
            continue;  // Пробуємо знову
        }

        // Якщо помилка якась інша (наприклад, "Access Denied") — тоді вже виходимо
        std::cout << "[!] Critical Connect Error: " << error << "\n";
        break;
    }

    std::cout << "[!] Failed to connect after 10 seconds timeout.\n";
    return false;
}

bool PipeClient::TryPassword(const std::string& login, const std::string& password) {
    // !!! ВИПРАВЛЕННЯ: АВТО-РЕКОНЕКТ !!!
    // Якщо з'єднання немає (розірвано на попередньому кроці), підключаємось знову
    if (!IsConnected()) {
        // Викликаємо Connect. Аргумент "." не важливий, бо ви захардкодили шлях всередині
        if (!Connect(".")) {
            // Помилка підключення вже виведеться у функції Connect
            return false;
        }
    }

    std::string message = login + " " + password;
    if (!SendData(message)) {
        Disconnect();
        return false;
    }

    DWORD response = 0; 
    if (!ReceiveResponse(response)) {
        Disconnect();
        return false;
    }

    bool isSuccess = (response == 1);
    Disconnect(); 
    return isSuccess;
}

void PipeClient::Disconnect() {
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }
}

bool PipeClient::IsConnected() const {
    return hPipe != INVALID_HANDLE_VALUE;
}

std::string PipeClient::GetLastErrorMsg() const {
    DWORD error = ::GetLastError();
    LPSTR errorText = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errorText, 0, NULL
    );

    std::string result;
    if (errorText) {
        result = std::string(errorText);
        LocalFree(errorText);
    } else {
        result = "Unknown error (" + std::to_string(error) + ")";
    }

    return result;
}

bool PipeClient::SendData(const std::string& data) {
    DWORD bytesWritten;
    if (!WriteFile(hPipe, data.c_str(), (DWORD)data.length(), &bytesWritten, NULL)) {
        std::cout << "[!] WriteFile failed: " << GetLastErrorMsg();
        Disconnect();
        return false;
    }

    if (bytesWritten != data.length()) {
        std::cout << "[!] WriteFile wrote " << bytesWritten << " bytes instead of " << data.length() << "\n";
        return false;
    }

    return true;
}

bool PipeClient::ReceiveResponse(DWORD& response) {
    DWORD bytesRead;
    
    if (!ReadFile(hPipe, &response, sizeof(DWORD), &bytesRead, NULL)) {
        std::cout << "[!] ReadFile failed: " << GetLastErrorMsg();
        Disconnect();
        return false;
    }

    if (bytesRead != sizeof(DWORD)) {
        std::cout << "[!] ReadFile read " << bytesRead << " bytes instead of " << sizeof(DWORD) << "\n";
        return false;
    }

    return true;
}
