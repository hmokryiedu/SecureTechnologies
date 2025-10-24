#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>

#pragma comment(lib, "comctl32.lib")

// Структура для збереження облікових даних
struct Credentials {
    std::string login;
    std::string password;
};

// Глобальні змінні
HWND hMainWnd, hLogEdit, hModeCombo, hStartBtn, hStopBtn;
std::vector<Credentials> credentialsList;
int maxLoginLen = 0, maxPasswordLen = 0;
bool serverRunning = false;
bool protectionMode = false;
std::mutex logMutex;
std::vector<std::thread> pipeThreads;
std::map<std::string, DWORD> loginAttempts; // логін -> кількість спроб
std::map<std::string, DWORD> loginBlockTime; // логін -> час блокування в мс
std::mutex attemptsMutex;

const int NUM_PIPES = 3;
const char* PIPE_NAME = "\\\\.\\pipe\\AuthPipe";

// Функція для додавання логу в текстове поле
void AddLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    int len = GetWindowTextLength(hLogEdit);
    SendMessage(hLogEdit, EM_SETSEL, len, len);
    std::string logMsg = message + "\r\n";
    SendMessage(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)logMsg.c_str());
}

// Читання файлу з обліковими даними
bool LoadCredentials(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        AddLog("Error: failed to open credentials file!");
        return false;
    }

    credentialsList.clear();
    loginAttempts.clear();
    loginBlockTime.clear();
    
    std::string line;
    // Перший рядок - максимальні довжини
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> maxLoginLen >> maxPasswordLen;
        AddLog("Max lengths: login=" + std::to_string(maxLoginLen) +
               ", password=" + std::to_string(maxPasswordLen));
    }

    // Читання пар логін-пароль
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Credentials cred;
        if (iss >> cred.login >> cred.password) {
            credentialsList.push_back(cred);
            loginAttempts[cred.login] = 0;
            loginBlockTime[cred.login] = 0;
        }
    }

    file.close();
    AddLog("Loaded " + std::to_string(credentialsList.size()) + " account(s)");
    return !credentialsList.empty();
}

// Перевірка облікових даних
bool CheckCredentials(const std::string& login, const std::string& password, std::string& resultMsg) {
    for (const auto& cred : credentialsList) {
        if (cred.login == login) {
            if (cred.password == password) {
                resultMsg = "Authentication successful for: " + login;
                return true;
            } else {
                resultMsg = "Invalid password for: " + login;
                return false;
            }
        }
    }
    resultMsg = "Login not found: " + login;
    return false;
}

// Обробка підключення клієнта
void HandleClient(HANDLE hPipe, int pipeId) {
    char buffer[512];
    DWORD bytesRead;
    
    // Очікування підключення клієнта
    BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    
    if (!connected) {
        AddLog("Pipe " + std::to_string(pipeId) + ": connection failed");
        CloseHandle(hPipe);
        return;
    }

    AddLog("Pipe " + std::to_string(pipeId) + ": client connected");
    
    // Читання логіна та пароля
    if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        std::string data(buffer);
        
        // Розділення логіна та пароля
        size_t spacePos = data.find(' ');
        if (spacePos != std::string::npos) {
            std::string login = data.substr(0, spacePos);
            std::string password = data.substr(spacePos + 1);
            
            // Режим захисту від злому
            if (protectionMode) {
                std::lock_guard<std::mutex> lock(attemptsMutex);
                loginAttempts[login]++;
                
                DWORD blockTime = loginBlockTime[login];
                if (blockTime > 0) {
                    AddLog("Pipe " + std::to_string(pipeId) + ": delay " +
                           std::to_string(blockTime) + " ms for " + login);
                    Sleep(blockTime);
                }
                
                // Збільшення часу блокування при кожній невдалій спробі
                loginBlockTime[login] += 1000; // +1 секунда за кожну спробу
            }
            
            // Перевірка облікових даних
            std::string resultMsg;
            bool success = CheckCredentials(login, password, resultMsg);
            AddLog("Pipe " + std::to_string(pipeId) + ": " + resultMsg);
            
            // Відправка результату
            char response = success ? '1' : '0';
            DWORD bytesWritten;
            WriteFile(hPipe, &response, 1, &bytesWritten, NULL);
            
            // Скидання лічильника при успішній аутентифікації
            if (success && protectionMode) {
                std::lock_guard<std::mutex> lock(attemptsMutex);
                loginAttempts[login] = 0;
                loginBlockTime[login] = 0;
            }
        }
    }
    
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    AddLog("Pipe " + std::to_string(pipeId) + ": client disconnected");
    CloseHandle(hPipe);
}

// Потік для роботи іменованого каналу
void PipeThread(int pipeId) {
    while (serverRunning) {
        // Створення екземпляра іменованого каналу
        HANDLE hPipe = CreateNamedPipeA(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            512, 512, 0, NULL
        );
        
        if (hPipe == INVALID_HANDLE_VALUE) {
            AddLog("Pipe " + std::to_string(pipeId) + ": creation failed");
            break;
        }
        
        HandleClient(hPipe, pipeId);
    }
    
    AddLog("Канал " + std::to_string(pipeId) + ": terminated");
}

// Запуск сервера
void StartServer() {
    if (serverRunning) return;
    
    serverRunning = true;
    protectionMode = (SendMessage(hModeCombo, CB_GETCURSEL, 0, 0) == 1);
    
    AddLog("=== Server started ===");
    AddLog(protectionMode ? "Mode: Anti-hacking" : "Mode: Normal");
    
    // Створення потоків для кожного екземпляру каналу
    for (int i = 0; i < NUM_PIPES; i++) {
        pipeThreads.push_back(std::thread(PipeThread, i + 1));
    }
    
    EnableWindow(hStartBtn, FALSE);
    EnableWindow(hStopBtn, TRUE);
    EnableWindow(hModeCombo, FALSE);
}

// Зупинка сервера
void StopServer() {
    if (!serverRunning) return;
    
    serverRunning = false;
    AddLog("=== Stopping server ===");
    
    // Очікування завершення всіх потоків
    for (auto& t : pipeThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    pipeThreads.clear();
    
    AddLog("=== Server stopped ===");
    
    EnableWindow(hStartBtn, TRUE);
    EnableWindow(hStopBtn, FALSE);
    EnableWindow(hModeCombo, TRUE);
    
    // Запит про продовження роботи
    int result = MessageBox(hMainWnd, 
        "Do you want to restart the server?",
        "Confirmation",
        MB_YESNO | MB_ICONQUESTION);
    
    if (result == IDYES) {
        StartServer();
    }
}

// Діалог вибору файлу
bool OpenFileDialog(std::string& filename) {
    char szFile[260] = {0};
    OPENFILENAME ofn = {0};
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileName(&ofn)) {
        filename = szFile;
        return true;
    }
    return false;
}

// Обробник повідомлень вікна
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Створення елементів інтерфейсу
            CreateWindow("STATIC", "Server mode:", WS_VISIBLE | WS_CHILD,
                10, 10, 100, 20, hwnd, NULL, NULL, NULL);
            
            hModeCombo = CreateWindow("COMBOBOX", "", 
                WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                120, 10, 200, 100, hwnd, (HMENU)1, NULL, NULL);
            SendMessage(hModeCombo, CB_ADDSTRING, 0, (LPARAM)"Normal");
            SendMessage(hModeCombo, CB_ADDSTRING, 0, (LPARAM)"Anti-hacking");
            SendMessage(hModeCombo, CB_SETCURSEL, 0, 0);
            
            hStartBtn = CreateWindow("BUTTON", "Load File and Start",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 40, 200, 30, hwnd, (HMENU)2, NULL, NULL);

            hStopBtn = CreateWindow("BUTTON", "Stop Server",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                220, 40, 150, 30, hwnd, (HMENU)3, NULL, NULL);
            EnableWindow(hStopBtn, FALSE);

            CreateWindow("STATIC", "Event Log:", WS_VISIBLE | WS_CHILD,
                10, 80, 100, 20, hwnd, NULL, NULL, NULL);
            
            hLogEdit = CreateWindow("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | 
                ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                10, 100, 760, 380, hwnd, NULL, NULL, NULL);
            
            break;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == 2) { // Кнопка запуску
                std::string filename;
                if (OpenFileDialog(filename)) {
                    AddLog("Selected file: " + filename);
                    if (LoadCredentials(filename)) {
                        StartServer();
                    }
                }
            } else if (LOWORD(wParam) == 3) { // Кнопка зупинки
                StopServer();
            }
            break;
        }
        
        case WM_CLOSE: {
            if (serverRunning) {
                int result = MessageBox(hwnd, 
                    "The server is still running. Are you sure you want to exit?",
                    "Exit confirmation",
                    MB_YESNO | MB_ICONWARNING);
                if (result == IDNO) return 0;
                StopServer();
            }
            DestroyWindow(hwnd);
            break;
        }
        
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Головна функція
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // Реєстрація класу вікна
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdminServerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Failed to register window class!", "Error", MB_ICONERROR);
        return 0;
    }
    
    // Створення головного вікна
    hMainWnd = CreateWindowEx(
        0, "AdminServerClass", "Administrator Server Application",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 550,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hMainWnd) {
        MessageBox(NULL, "Failed to create main window!", "Error", MB_ICONERROR);
        return 0;
    }
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    // Цикл обробки повідомлень
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}