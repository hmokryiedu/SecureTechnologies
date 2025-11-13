#pragma once

#include <windows.h>
#include <string>

class PipeClient {
private:
    HANDLE hPipe;
    std::string pipeName;
    static const int BUFFER_SIZE = 512;

public:
    PipeClient(const std::string& name = "\\\\ServerName\\pipe\\AuthPipe");
    ~PipeClient();

    bool Connect(const std::string& computerName = ".");
    bool TryPassword(const std::string& login, const std::string& password);
    void Disconnect();
    bool IsConnected() const;
    std::string GetLastErrorMsg() const;

private:
    bool SendData(const std::string& data);
    bool ReceiveResponse(char& response);
};
