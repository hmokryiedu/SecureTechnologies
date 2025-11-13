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
        std::cout << "[!] Not connected to server\n";
        return false;
    }

    std::string message = login + " " + password;
    if (!SendData(message)) {
        return false;
    }

    char response;
    if (!ReceiveResponse(response)) {
        return false;
    }

    return (response == '1');
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

bool PipeClient::ReceiveResponse(char& response) {
    DWORD bytesRead;
    if (!ReadFile(hPipe, &response, 1, &bytesRead, NULL)) {
        std::cout << "[!] ReadFile failed: " << GetLastErrorMsg();
        Disconnect();
        return false;
    }

    if (bytesRead != 1) {
        std::cout << "[!] ReadFile read " << bytesRead << " bytes instead of 1\n";
        return false;
    }

    return true;
}
