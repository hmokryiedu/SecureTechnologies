#include "PipeClient.h"
#include <iostream>
#include <sstream>

PipeClient::PipeClient(const std::string& name)
    : hPipe(INVALID_HANDLE_VALUE), pipeName(name), lastComputerName(".") {
}

PipeClient::~PipeClient() {
    Disconnect();
}

bool PipeClient::Connect(const std::string& computerName) {
    lastComputerName = computerName;
    std::string fullPipeName;
    if (computerName == ".") {
        fullPipeName = "\\\\.\\pipe\\AuthPipe";
    } else {
        fullPipeName = "\\\\" + computerName + "\\pipe\\AuthPipe";
    }

    hPipe = CreateFileA(fullPipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe != INVALID_HANDLE_VALUE) {
        return true;
    }

    if (::GetLastError() == ERROR_PIPE_BUSY) {
        std::cout << "[*] Pipe busy, waiting...\n";
        if (!WaitNamedPipeA(fullPipeName.c_str(), 5000)) {
            std::cout << "[!] Timeout waiting for pipe\n";
            return false;
        }

        hPipe = CreateFileA(fullPipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) {
            return true;
        }
    }

    std::cout << "[!] Failed to connect to pipe: " << GetLastErrorMsg() << "\n";
    return false;
}

bool PipeClient::TryPassword(const std::string& login, const std::string& password) {
    if (!IsConnected()) {
        std::string target = lastComputerName.empty() ? "." : lastComputerName;
        if (!Connect(target)) {
            std::cout << "[!] Connection failed inside TryPassword\n";
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

bool PipeClient::Reconnect() {
    Disconnect();
    return Connect(lastComputerName);
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
        std::cout << "[!] WriteFile failed, attempting reconnection...\n";
        if (Reconnect()) {
            std::cout << "[*] Reconnected, retrying...\n";
            if (!WriteFile(hPipe, data.c_str(), (DWORD)data.length(), &bytesWritten, NULL)) {
                std::cout << "[!] WriteFile failed after reconnection: " << GetLastErrorMsg() << "\n";
                Disconnect();
                return false;
            }
        } else {
            std::cout << "[!] Failed to reconnect to server\n";
            return false;
        }
    }

    if (bytesWritten != data.length()) {
        std::cout << "[!] WriteFile wrote " << bytesWritten << " bytes instead of " << data.length() << "\n";
        return false;
    }

    return true;
}

bool PipeClient::ReceiveResponse(DWORD& response) {
    DWORD bytesRead;
    // Читаємо sizeof(DWORD) (4 байти)
    if (!ReadFile(hPipe, &response, sizeof(DWORD), &bytesRead, NULL)) {
        std::cout << "[!] ReadFile failed, attempting reconnection...\n";
        if (Reconnect()) {
            std::cout << "[*] Reconnected, retrying...\n";
            // Тут теж читаємо sizeof(DWORD)
            if (!ReadFile(hPipe, &response, sizeof(DWORD), &bytesRead, NULL)) {
                std::cout << "[!] ReadFile failed after reconnection: " << GetLastErrorMsg() << "\n";
                Disconnect();
                return false;
            }
        } else {
            std::cout << "[!] Failed to reconnect to server\n";
            return false;
        }
    }

    // Перевіряємо, чи прочитали ми 4 байти
    if (bytesRead != sizeof(DWORD)) {
        std::cout << "[!] ReadFile read " << bytesRead << " bytes instead of " << sizeof(DWORD) << "\n";
        return false;
    }

    return true;
}
