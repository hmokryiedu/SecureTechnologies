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
    // Always start fresh - connect for each attempt
    // This is more reliable with the current server architecture
    if (!Connect(lastComputerName.empty() ? "." : lastComputerName)) {
        return false;
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

    // Disconnect after each attempt - server expects this
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
    
    if (!ReadFile(hPipe, &response, sizeof(DWORD), &bytesRead, NULL)) {
        DWORD error = ::GetLastError();
        
        // Enhanced error reporting (Issue #10)
        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
            std::cout << "[!] Server disconnected (Error " << error << ")\n";
        } else if (error == ERROR_INVALID_HANDLE) {
            std::cout << "[!] Invalid pipe handle (Error " << error << ")\n";
        } else {
            std::cout << "[!] ReadFile failed (Error " << error << "): " << GetLastErrorMsg() << "\n";
        }
        
        // Try to reconnect for transient errors
        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
            std::cout << "[*] Attempting reconnection...\n";
            if (Reconnect()) {
                std::cout << "[*] Reconnected, retrying...\n";
                if (!ReadFile(hPipe, &response, sizeof(DWORD), &bytesRead, NULL)) {
                    DWORD retryError = ::GetLastError();
                    std::cout << "[!] ReadFile failed after reconnection (Error " << retryError << "): " 
                             << GetLastErrorMsg() << "\n";
                    Disconnect();
                    return false;
                }
            } else {
                std::cout << "[!] Failed to reconnect to server\n";
                return false;
            }
        } else {
            Disconnect();
            return false;
        }
    }

    // Validate response size
    if (bytesRead != sizeof(DWORD)) {
        std::cout << "[!] Incomplete response: received " << bytesRead 
                  << " bytes, expected " << sizeof(DWORD) << " bytes\n";
        return false;
    }

    return true;
}
