#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <mutex>
#include <commdlg.h>
#include "PipeServer.h"
#include "PerPipeStruct.h"
#include "list.h"

// Structure for credentials
struct Credentials {
    std::string login;
    std::string password;
};

// User-specific data for each pipe
struct UserAuthData {
    DWORD lastAttemptTime;
    DWORD blockDuration;
    int attemptCount;
    
    UserAuthData() : lastAttemptTime(0), blockDuration(0), attemptCount(0) {}
};

// Global variables
List<Credentials> credentialsList;
int maxLoginLen = 0, maxPasswordLen = 0;
bool protectionMode = false;
std::map<std::string, int> loginAttempts;
std::map<std::string, DWORD> loginBlockTime;
std::mutex dataMutex;

HWND hMainWnd, hLogEdit, hModeCombo, hStartBtn, hStopBtn, hStatusLabel;
PipeServer<UserAuthData>* pServer = nullptr;

// Function for adding a log message
void AddLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(dataMutex);
    
    if (hLogEdit) {
        int len = GetWindowTextLength(hLogEdit);
        SendMessage(hLogEdit, EM_SETSEL, len, len);
        
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeStr[64];
        sprintf(timeStr, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
        
        std::string logMsg = timeStr + message + "\r\n";
        SendMessage(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)logMsg.c_str());
    } else {
        std::cout << message << std::endl;
    }
}

// Update server status label
void UpdateStatus(const std::string& status) {
    if (hStatusLabel) {
        SetWindowTextA(hStatusLabel, status.c_str());
    }
}

// Read credentials file
bool LoadCredentials(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        AddLog("ERROR: Failed to open the file!");
        return false;
    }
    
    credentialsList.clear();
    loginAttempts.clear();
    loginBlockTime.clear();
    
    std::string line;
    
    // The first line — maximum login/password lengths
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> maxLoginLen >> maxPasswordLen;
        AddLog("Max lengths: login=" + std::to_string(maxLoginLen) + 
               ", password=" + std::to_string(maxPasswordLen));
    }
    
    // Read login-password pairs
    int count = 0;
    while (std::getline(file, line) && count < 20) {
        std::istringstream iss(line);
        Credentials cred;
        if (iss >> cred.login >> cred.password) {
            credentialsList.push_back(cred);
            loginAttempts[cred.login] = 0;
            loginBlockTime[cred.login] = 0;
            count++;
        }
    }
    
    file.close();
    
    if (count == 0) {
        AddLog("ERROR: No accounts found!");
        return false;
    }
    
    AddLog("Loaded " + std::to_string(count) + " accounts");
    return true;
}

// Verify user credentials
bool CheckCredentials(const std::string& login, const std::string& password) {
    for (auto it = credentialsList.begin(); it != credentialsList.end(); ++it) {
        if (it->login == login && it->password == password) {
            return true;
        }
    }
    return false;
}

// Callback when a client connects
void OnClientConnect(PerPipeStruct<UserAuthData>* ps, void* userData) {
    AddLog("Pipe #" + std::to_string(ps->pipeId) + ": Client connected");
    UpdateStatus("Active connections: " + std::to_string(pServer->GetPipeCount()));
    
    if (!ps->userData) {
        ps->userData = new UserAuthData();
    }
}

// Callback when data is received
void OnClientRead(PerPipeStruct<UserAuthData>* ps, void* userData) {
    std::string data(ps->buffer);
    
    // Split login and password
    size_t spacePos = data.find(' ');
    if (spacePos == std::string::npos) {
        AddLog("Pipe #" + std::to_string(ps->pipeId) + ": Invalid data format");
        char response = '0';
        pServer->WriteResponse(ps, &response, 1);
        return;
    }
    
    std::string login = data.substr(0, spacePos);
    std::string password = data.substr(spacePos + 1);
    
    AddLog("Pipe #" + std::to_string(ps->pipeId) + ": Login attempt for user '" + login + "'");
    
    // Anti-hacking protection mode
    if (protectionMode && ps->userData) {
        std::lock_guard<std::mutex> lock(dataMutex);
        
        DWORD currentTime = GetTickCount();
        DWORD blockTime = loginBlockTime[login];
        
        if (blockTime > 0) {
            AddLog("Pipe #" + std::to_string(ps->pipeId) + 
                   ": Delay " + std::to_string(blockTime) + " ms for '" + login + "'");
            Sleep(blockTime);
        }
    }
    
    // Check credentials
    bool success = CheckCredentials(login, password);
    
    if (success) {
        AddLog("Pipe #" + std::to_string(ps->pipeId) + ": ✓ Successful authentication for '" + login + "'");
        
        // Reset attempt counter
        if (protectionMode) {
            std::lock_guard<std::mutex> lock(dataMutex);
            loginAttempts[login] = 0;
            loginBlockTime[login] = 0;
        }
    } else {
        AddLog("Pipe #" + std::to_string(ps->pipeId) + ": ✗ Invalid password for '" + login + "'");
        
        // Increase blocking time
        if (protectionMode) {
            std::lock_guard<std::mutex> lock(dataMutex);
            loginAttempts[login]++;
            loginBlockTime[login] += 1000; // +1 second delay
            AddLog("  Attempts for '" + login + "': " + std::to_string(loginAttempts[login]));
        }
    }
    
    // Send response to client
    char response = success ? '1' : '0';
    pServer->WriteResponse(ps, &response, 1);
}

// Callback when a client disconnects
void OnClientDisconnect(PerPipeStruct<UserAuthData>* ps, void* userData) {
    AddLog("Pipe #" + std::to_string(ps->pipeId) + ": Client disconnected");
}

// File selection dialog
bool OpenFileDialog(std::string& filename) {
    char szFile[260] = {0};
    OPENFILENAME ofn = {0};
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn)) {
        filename = szFile;
        return true;
    }
    return false;
}

// Start the server
void StartServer() {
    if (pServer && pServer->IsRunning()) return;
    
    std::string filename;
    if (!OpenFileDialog(filename)) {
        return;
    }
    
    AddLog("Selected file: " + filename);
    if (!LoadCredentials(filename)) {
        return;
    }
    
    protectionMode = (SendMessage(hModeCombo, CB_GETCURSEL, 0, 0) == 1);
    
    if (!pServer) {
        pServer = new PipeServer<UserAuthData>("\\\\ServerName\\pipe\\AuthPipe", 3);
        pServer->SetConnectCallback(OnClientConnect);
        pServer->SetReadCallback(OnClientRead);
        pServer->SetDisconnectCallback(OnClientDisconnect);
    }
    
    if (pServer->Start()) {
        AddLog("========================================");
        AddLog("SERVER STARTED");
        AddLog("Mode: " + std::string(protectionMode ? "Anti-hacking mode" : "Normal mode"));
        AddLog("Pipe name: \\\\ServerName\\pipe\\AuthPipe");
        AddLog("Instances: 3");
        AddLog("========================================");
        
        EnableWindow(hStartBtn, FALSE);
        EnableWindow(hStopBtn, TRUE);
        EnableWindow(hModeCombo, FALSE);
        UpdateStatus("Server is running");
    } else {
        AddLog("ERROR: Failed to start server!");
    }
}

// Stop the server
void StopServer() {
    if (!pServer || !pServer->IsRunning()) return;
    
    AddLog("========================================");
    AddLog("STOPPING SERVER...");
    
    pServer->Stop();
    
    AddLog("SERVER STOPPED");
    AddLog("========================================");
    
    EnableWindow(hStartBtn, TRUE);
    EnableWindow(hStopBtn, FALSE);
    EnableWindow(hModeCombo, TRUE);
    UpdateStatus("Server stopped");
    
    // Ask if user wants to restart the server
    int result = MessageBox(hMainWnd, 
        "Restart the server?", 
        "Confirmation", 
        MB_YESNO | MB_ICONQUESTION);
    
    if (result == IDYES) {
        StartServer();
    }
}

// Message handler
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Operating mode
            CreateWindow("STATIC", "Operating mode:", WS_VISIBLE | WS_CHILD,
                10, 10, 100, 30, hwnd, NULL, NULL, NULL);
            
            hModeCombo = CreateWindow("COMBOBOX", "", 
                WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                120, 10, 200, 100, hwnd, (HMENU)1, NULL, NULL);
            SendMessage(hModeCombo, CB_ADDSTRING, 0, (LPARAM)"Normal");
            SendMessage(hModeCombo, CB_ADDSTRING, 0, (LPARAM)"Anti-hacking");
            SendMessage(hModeCombo, CB_SETCURSEL, 0, 0);
            
            // Buttons
            hStartBtn = CreateWindow("BUTTON", "Load and Start",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 40, 180, 30, hwnd, (HMENU)2, NULL, NULL);
            
            hStopBtn = CreateWindow("BUTTON", "Stop",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                200, 40, 120, 30, hwnd, (HMENU)3, NULL, NULL);
            EnableWindow(hStopBtn, FALSE);
            
            // Status
            hStatusLabel = CreateWindow("STATIC", "Status: Waiting to start",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                340, 45, 400, 20, hwnd, NULL, NULL, NULL);
            
            // Log
            CreateWindow("STATIC", "Event log:", WS_VISIBLE | WS_CHILD,
                10, 80, 100, 20, hwnd, NULL, NULL, NULL);
            
            hLogEdit = CreateWindow("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | 
                ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                10, 105, 760, 375, hwnd, NULL, NULL, NULL);
            
            // Timer for processing server events
            SetTimer(hwnd, 1, 50, NULL);
            
            break;
        }
        
        case WM_TIMER: {
            if (pServer && pServer->IsRunning()) {
                pServer->ProcessEvents(10);
            }
            break;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 2) {
                StartServer();
            } else if (LOWORD(wParam) == 3) {
                StopServer();
            }
            break;
        }
        
        case WM_CLOSE: {
            if (pServer && pServer->IsRunning()) {
                int result = MessageBox(hwnd, 
                    "The server is running. Are you sure you want to exit?",
                    "Confirmation", 
                    MB_YESNO | MB_ICONWARNING);
                if (result == IDNO) return 0;
                StopServer();
            }
            DestroyWindow(hwnd);
            break;
        }
        
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            if (pServer) {
                delete pServer;
                pServer = nullptr;
            }
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Main function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ServerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Registration error!", "Error", MB_ICONERROR);
        return 0;
    }
    
    // Create window
    hMainWnd = CreateWindowEx(0, "ServerClass", 
        "Administrator Server Application (using PipeServer)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 550,
        NULL, NULL, hInstance, NULL);
    
    if (!hMainWnd) {
        MessageBox(NULL, "Failed to create window!", "Error", MB_ICONERROR);
        return 0;
    }
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
