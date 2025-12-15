# Серверное приложение - Детальный технический обзор

## Оглавление
1. [Общая архитектура](#1-общая-архитектура)
2. [Точка входа и главное окно](#2-точка-входа-и-главное-окно)
3. [Управление пользователями](#3-управление-пользователями)
4. [Многопоточный сервер Named Pipes](#4-многопоточный-сервер-named-pipes)
5. [Система противодействия взлому](#5-система-противодействия-взлому)
6. [Протокол обмена данными](#6-протокол-обмена-данными)
7. [Соответствие требованиям](#7-соответствие-требованиям)

---

## 1. Общая архитектура

### 1.1 Структура проекта

```
server/
├── test.cpp              # Точка входа (WinMain), создание GUI окна
├── ServerContext.h       # Инкапсуляция состояния сервера
├── ServerConstants.h     # Конфигурационные константы
├── PipeServer.h          # Шаблонный класс работы с Named Pipes (асинхронный режим)
├── PerPipeStruct.h       # Шаблонный класс данных для каждого экземпляра канала
├── list.h                # Класс UserList для работы со списком пользователей
└── passwords.txt         # Тестовый файл с логинами/паролями
```

### 1.2 Диаграмма классов

```
┌────────────────────────────────────────────────────────────────┐
│                       ServerContext                            │
│────────────────────────────────────────────────────────────────│
│ - users: UserList          # Список пользователей              │
│ - server: PipeServer       # Экземпляр сервера каналов         │
│────────────────────────────────────────────────────────────────│
│ + GetUsers(): UserList&    # Доступ к списку пользователей     │
│ + GetServer(): PipeServer& # Доступ к серверу                  │
│ + operator= (deleted)      # Запрет копирования                │
└────────────────────────────────────────────────────────────────┘
                          │ содержит
         ┌────────────────┴────────────────┐
         ▼                                 ▼
┌─────────────────────────┐    ┌────────────────────────────────┐
│      UserList           │    │        PipeServer              │
│─────────────────────────│    │────────────────────────────────│
│ - users: vector<User>   │    │ - threads: vector<HANDLE>      │
│ - blockedUsers: map     │    │ - isRunning: bool              │
│ - protectionMode: bool  │    │ - hShutdownEvent: HANDLE       │
│ - isServerRunning: bool │    │ - userList: UserList*          │
│ - maxLoginLen: int      │    │────────────────────────────────│
│ - maxPassLen: int       │    │ + Start(numPipes, users, hGui) │
│ - blockedUsersLock:     │    │ + Stop()                       │
│   CRITICAL_SECTION      │    │ - PipeInstanceThread() [static]│
│─────────────────────────│    │ - Log() [static]               │
│ + Load(hwnd): bool      │    └────────────────────────────────┘
│ + LoadFromFile(): bool  │
│ + CheckUser(): int      │
│ + SetProtection(): void │
│ + Count(): size_t       │
└─────────────────────────┘

┌────────────────────────────────────────────────────────────────┐
│                       PerPipeData                              │
│────────────────────────────────────────────────────────────────│
│ - hPipe: HANDLE            # Дескриптор канала                 │
│ - pipeId: int              # Идентификатор экземпляра          │
│ - hGui: HWND               # Дескриптор GUI окна               │
│ - userList: UserList*      # Указатель на базу пользователей   │
│ - oOverlap: OVERLAPPED     # Структура асинхронных операций    │
│ - hShutdownEvent: HANDLE   # Событие завершения                │
│────────────────────────────────────────────────────────────────│
│ + GetPipe(), SetPipe()     # Аксессоры для hPipe               │
│ + GetOverlap()             # Доступ к OVERLAPPED               │
│ + GetUserList()            # Доступ к списку пользователей     │
│ + GetShutdownEvent()       # Доступ к событию остановки        │
└────────────────────────────────────────────────────────────────┘
```

### 1.3 Потоковая модель

```
┌──────────────────────────────────────────────────────────────────┐
│                    ГЛАВНЫЙ ПОТОК (GUI)                           │
│  • Обработка сообщений Windows                                   │
│  • Реагирование на нажатие кнопок                                │
│  • Вывод логов в ListBox                                         │
│  • Управление жизненным циклом сервера                           │
└────────────────────────────┬─────────────────────────────────────┘
                             │ Start()
                             ▼
┌──────────────────────────────────────────────────────────────────┐
│                  РАБОЧИЕ ПОТОКИ (N штук)                         │
│ ┌──────────────┐ ┌──────────────┐     ┌──────────────┐          │
│ │  Thread #0   │ │  Thread #1   │ ... │  Thread #N   │          │
│ │ PipeInstance │ │ PipeInstance │     │ PipeInstance │          │
│ └──────────────┘ └──────────────┘     └──────────────┘          │
│  • Создание именованного канала                                  │
│  • Ожидание подключения клиентов (асинхронно)                    │
│  • Чтение/запись данных (асинхронно)                             │
│  • Проверка логина/пароля                                        │
│  • Отправка результата                                           │
└──────────────────────────────────────────────────────────────────┘
```

---

## 2. Точка входа и главное окно

### 2.1 Файл `test.cpp`

**Функция `WinMain`** - точка входа приложения с GUI:

```cpp
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow) {
    // 1. Регистрация класса окна
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;           // Функция обработки сообщений
    wc.hInstance = hInstance;
    wc.lpszClassName = "LabServerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    // 2. Создание главного окна
    HWND hwnd = CreateWindow(CLASS_NAME, "Password Server (Admin)", 
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
                             NULL, NULL, hInstance, NULL);

    // 3. Создание и привязка контекста сервера к окну
    ServerContext* context = new ServerContext();
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(context));

    // 4. Показ окна и запуск цикла сообщений
    ShowWindow(hwnd, nCmdShow);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
```

### 2.2 Элементы управления GUI

| ID | Тип | Назначение |
|----|-----|------------|
| `IDC_LOAD_BTN` (101) | Button | Загрузка файла пользователей |
| `IDC_START_BTN` (102) | Button | Запуск сервера |
| `IDC_PROTECT_CHK` (103) | CheckBox | Включение режима защиты от взлома |
| `IDC_LOG_LIST` (104) | ListBox | Вывод логов работы сервера |

### 2.3 Обработчик сообщений `WindowProc`

```cpp
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ServerContext* context = reinterpret_cast<ServerContext*>(
        GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (uMsg) {
    case WM_CREATE:
        // Создание кнопок, чекбокса и списка логов при инициализации окна
        CreateWindow("BUTTON", "Load Users File", ...);
        CreateWindow("BUTTON", "Anti-Brute-Force Mode", ...);
        CreateWindow("BUTTON", "Start Server", ...);
        CreateWindowEx(..., "LISTBOX", ...);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LOAD_BTN:
            // Загрузка файла пользователей через GetOpenFileName
            if (context->GetUsers().Load(hwnd)) {
                // Вывод количества загруженных пользователей
            }
            break;

        case IDC_PROTECT_CHK:
            // Переключение режима защиты
            BOOL checked = IsDlgButtonChecked(hwnd, IDC_PROTECT_CHK);
            context->GetUsers().SetProtection(checked == BST_CHECKED);
            break;

        case IDC_START_BTN:
            // Запуск сервера
            if (context->GetUsers().Count() == 0) {
                MessageBox(hwnd, "Please load users file first!", "Error", MB_ICONERROR);
            } else {
                unsigned int hwThreads = std::thread::hardware_concurrency() * 2;
                context->GetServer().Start(hwThreads, &context->GetUsers(), hwnd);
                EnableWindow(GetDlgItem(hwnd, IDC_START_BTN), FALSE);
            }
            break;
        }
        break;

    case WM_LOG_MSG:
        // Получение лога от рабочих потоков (SendMessage - синхронно)
        const char* text = (const char*)wParam;
        std::string logText(text, lParam);
        SendDlgItemMessage(hwnd, IDC_LOG_LIST, LB_ADDSTRING, 0, (LPARAM)logText.c_str());
        break;

    case WM_DESTROY:
        // Корректное завершение всех потоков при закрытии окна
        context->GetServer().Stop();
        delete context;
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
```

### 2.4 Жизненный цикл GUI

```
┌─────────────────────────────────────────────────────────────────┐
│                    ЖИЗНЕННЫЙ ЦИКЛ СЕРВЕРА                       │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   WM_CREATE           │
                    │   Создание элементов  │
                    │   управления          │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   Пользователь:       │
                    │   "Load Users File"   │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   GetOpenFileName()   │
                    │   LoadFromFile()      │
                    │   → "Loaded N users"  │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   Пользователь:       │
                    │   [✓] Anti-Brute-Force│
                    │   (опционально)       │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   Пользователь:       │
                    │   "Start Server"      │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   PipeServer::Start() │
                    │   Запуск N потоков    │
                    │   → "Server ON"       │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   Обработка клиентов  │
                    │   WM_LOG_MSG          │
                    │   → логи в ListBox    │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   WM_DESTROY          │
                    │   PipeServer::Stop()  │
                    │   Cleanup             │
                    └───────────────────────┘
```

---

## 3. Управление пользователями

### 3.1 Класс `UserList` (файл `list.h`)

**Внутренняя структура:**

```cpp
class UserList {
private:
    struct User {
        string login;     // Логин пользователя
        string password;  // Пароль пользователя
    };

    vector<User> users;                    // Список всех пользователей
    map<string, DWORD> blockedUsers;       // Заблокированные логины: логин → время разблокировки
    bool protectionMode;                   // Флаг режима защиты
    bool isServerRunning;                  // Флаг работы сервера
    int maxLoginLen;                       // Максимальная длина логина (из файла)
    int maxPassLen;                        // Максимальная длина пароля (из файла)
    CRITICAL_SECTION blockedUsersLock;     // Синхронизация доступа к blockedUsers
};
```

### 3.2 Формат файла пользователей

**Структура файла `passwords.txt`:**

```
20 20                          # Строка 1: макс_длина_логина макс_длина_пароля
david qwe                      # Строка 2+: логин пароль
amanda 123456
michael password
williams abcdef
richard anna2000
john ivan1995
jessica maksym
miller olga88
davis 12345
laura qweasd
garcia abc123
...
```

### 3.3 Метод `Load(HWND hwnd)`

**Алгоритм загрузки:**

```cpp
bool Load(HWND hwnd) {
    // 1. Проверка: сервер НЕ должен быть запущен
    if (isServerRunning) {
        MessageBoxA(hwnd, "Cannot load users while server is running!", 
                   "Server Running", MB_ICONWARNING);
        return false;
    }
    
    // 2. Открытие диалога выбора файла
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // 3. Если файл выбран - загрузка
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return LoadFromFile(ofn.lpstrFile);
    }
    return false;
}
```

### 3.4 Метод `LoadFromFile(filepath)`

```cpp
bool LoadFromFile(const string& filepath) {
    ifstream file(filepath);
    if (!file.is_open()) return false;

    users.clear();
    string line;

    // 1. Чтение первой строки: максимальные длины
    if (getline(file, line)) {
        stringstream ss(line);
        ss >> maxLoginLen >> maxPassLen;
    }

    // 2. Чтение пар логин/пароль
    while (getline(file, line)) {
        stringstream ss(line);
        string login, password;
        if (ss >> login >> password) {
            users.push_back({ login, password });
        }
    }
    return true;
}
```

### 3.5 Метод `CheckUser(login, pass)`

**Возвращаемые значения:**
- `1` = Успешная авторизация
- `0` = Неверный пароль или пользователь не найден
- `-1` = Логин заблокирован (в режиме защиты)

**Потокобезопасный алгоритм:**

```cpp
int CheckUser(const string& login, const string& pass) {
    // 1. ПРОВЕРКА БЛОКИРОВКИ (только в режиме защиты)
    if (protectionMode) {
        EnterCriticalSection(&blockedUsersLock);
        
        DWORD currentTime = GetTickCount();
        if (blockedUsers.count(login)) {
            if (currentTime < blockedUsers[login]) {
                // Логин ещё заблокирован
                LeaveCriticalSection(&blockedUsersLock);
                return -1;  // ЗАБЛОКИРОВАН
            } else {
                // Время блокировки истекло
                blockedUsers.erase(login);
            }
        }
        
        LeaveCriticalSection(&blockedUsersLock);
    }

    // 2. ПОИСК ПОЛЬЗОВАТЕЛЯ
    for (const auto& user : users) {
        if (user.login == login) {
            if (user.password == pass) {
                // 2.1 Успешная авторизация - снимаем блокировку
                if (protectionMode) {
                    EnterCriticalSection(&blockedUsersLock);
                    blockedUsers.erase(login);
                    LeaveCriticalSection(&blockedUsersLock);
                }
                return 1;  // УСПЕХ
            } else {
                // 2.2 Неверный пароль - блокируем на N мс
                if (protectionMode) {
                    EnterCriticalSection(&blockedUsersLock);
                    blockedUsers[login] = GetTickCount() + ServerConfig::BLOCK_DURATION_MS;
                    LeaveCriticalSection(&blockedUsersLock);
                }
                return 0;  // НЕВЕРНЫЙ ПАРОЛЬ
            }
        }
    }
    
    return 0;  // ПОЛЬЗОВАТЕЛЬ НЕ НАЙДЕН
}
```

### 3.6 Синхронизация доступа

**Использование CRITICAL_SECTION:**

```cpp
// Инициализация в конструкторе
UserList() {
    InitializeCriticalSection(&blockedUsersLock);
}

// Освобождение в деструкторе
~UserList() {
    DeleteCriticalSection(&blockedUsersLock);
}

// Паттерн использования:
EnterCriticalSection(&blockedUsersLock);
// ... критическая секция (доступ к blockedUsers) ...
LeaveCriticalSection(&blockedUsersLock);
```

---

## 4. Многопоточный сервер Named Pipes

### 4.1 Класс `PipeServer` (файл `PipeServer.h`)

**Конфигурация:**

```cpp
#define PIPE_NAME "\\\\.\\pipe\\AuthPipe"     // Имя канала
#define BUFFER_SIZE ServerConfig::PIPE_BUFFER_SIZE  // 512 байт

class PipeServer {
private:
    vector<HANDLE> threads;       // Дескрипторы рабочих потоков
    bool isRunning;               // Флаг работы сервера
    HANDLE hShutdownEvent;        // Событие для graceful shutdown
    UserList* userList;           // Указатель на список пользователей
};
```

### 4.2 Метод `Start(numPipes, users, hGui)`

```cpp
void Start(int numPipes, UserList* uList, HWND hGui) {
    isRunning = true;
    userList = uList;
    userList->SetServerRunning(true);  // Блокируем загрузку новых пользователей
    
    // 1. Создание события для остановки (manual-reset, initially non-signaled)
    hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    // 2. Запуск N потоков обработки каналов
    for (int i = 0; i < numPipes; i++) {
        // Создание контекста для потока
        PerPipeData* data = new PerPipeData(i + 1, hGui, uList, hShutdownEvent);
        
        // Запуск потока
        HANDLE hThread = CreateThread(NULL, 0, PipeInstanceThread, data, 0, NULL);
        if (hThread) {
            threads.push_back(hThread);
        }
    }
    
    Log(hGui, "Async Server ON. Pipe: " + string(PIPE_NAME));
}
```

### 4.3 Метод `Stop()`

**Graceful shutdown всех потоков:**

```cpp
void Stop() {
    if (!isRunning) return;
    
    isRunning = false;
    
    if (hShutdownEvent) {
        // 1. Сигнализируем всем потокам о необходимости завершения
        SetEvent(hShutdownEvent);
        
        // 2. Ожидание завершения всех потоков
        if (threads.size() > 0) {
            DWORD result = WaitForMultipleObjects(
                (DWORD)threads.size(),
                threads.data(),
                TRUE,  // Ждём ВСЕ потоки
                ServerConfig::SHUTDOWN_TIMEOUT_MS  // 5000 мс таймаут
            );
            
            // 3. Освобождение дескрипторов потоков
            for (HANDLE h : threads) {
                CloseHandle(h);
            }
            threads.clear();
        }
        
        CloseHandle(hShutdownEvent);
        hShutdownEvent = NULL;
    }
    
    // 4. Разрешаем модификацию списка пользователей
    if (userList) {
        userList->SetServerRunning(false);
        userList = nullptr;
    }
}
```

### 4.4 Статический метод `PipeInstanceThread`

**Главный цикл обработки клиентов:**

```cpp
static DWORD WINAPI PipeInstanceThread(LPVOID lpvParam) {
    PerPipeData* data = (PerPipeData*)lpvParam;
    char buffer[BUFFER_SIZE];
    DWORD bytesTransferred;
    BOOL success;

    // 1. ИНИЦИАЛИЗАЦИЯ OVERLAPPED СОБЫТИЯ
    // Manual Reset - важно для асинхронных операций!
    data->GetOverlap()->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (data->GetOverlap()->hEvent == NULL) {
        Log(data->GetGui(), "Error creating event.");
        delete data;
        return 0;
    }

    // 2. СОЗДАНИЕ ИМЕНОВАННОГО КАНАЛА
    data->SetPipe(CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // Двунаправленный + асинхронный
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,  // Неограниченное количество экземпляров
        BUFFER_SIZE, BUFFER_SIZE,
        0,      // Таймаут по умолчанию
        NULL    // Атрибуты безопасности
    ));

    if (data->GetPipe() == INVALID_HANDLE_VALUE) {
        Log(data->GetGui(), "Error creating pipe.");
        CloseHandle(data->GetOverlap()->hEvent);
        delete data;
        return 0;
    }

    // 3. ГЛАВНЫЙ ЦИКЛ ОБРАБОТКИ
    while (WaitForSingleObject(data->GetShutdownEvent(), 0) == WAIT_TIMEOUT) {
        // ... обработка клиентов (см. секцию 4.5) ...
    }

    // 4. ОСВОБОЖДЕНИЕ РЕСУРСОВ
    CloseHandle(data->GetOverlap()->hEvent);
    CloseHandle(data->GetPipe());
    delete data;
    return 0;
}
```

### 4.5 Обработка одного клиента (внутри главного цикла)

```cpp
// 3.1 АСИНХРОННОЕ ПОДКЛЮЧЕНИЕ
ResetEvent(data->GetOverlap()->hEvent);

BOOL connected = ConnectNamedPipe(data->GetPipe(), data->GetOverlap());

if (!connected) {
    DWORD error = GetLastError();
    
    if (error == ERROR_IO_PENDING) {
        // Ожидание асинхронного завершения подключения
        if (!GetOverlappedResult(data->GetPipe(), data->GetOverlap(), 
                                  &bytesTransferred, TRUE)) {
            DisconnectNamedPipe(data->GetPipe());
            continue;  // Повторная попытка
        }
    }
    else if (error == ERROR_PIPE_CONNECTED) {
        // Клиент уже подключён (race condition)
    }
    else {
        DisconnectNamedPipe(data->GetPipe());
        continue;
    }
}

// --- КЛИЕНТ ПОДКЛЮЧЁН ---

// 3.2 АСИНХРОННОЕ ЧТЕНИЕ ДАННЫХ
ZeroMemory(buffer, BUFFER_SIZE);
ResetEvent(data->GetOverlap()->hEvent);

success = ReadFile(data->GetPipe(), buffer, BUFFER_SIZE - 1, 
                   &bytesTransferred, data->GetOverlap());

if (!success && GetLastError() == ERROR_IO_PENDING) {
    success = GetOverlappedResult(data->GetPipe(), data->GetOverlap(), 
                                   &bytesTransferred, TRUE);
}

if (success && bytesTransferred > 0) {
    // Защита от переполнения буфера
    if (bytesTransferred >= BUFFER_SIZE) {
        bytesTransferred = BUFFER_SIZE - 1;
    }
    buffer[bytesTransferred] = '\0';
    
    // 3.3 ПАРСИНГ СООБЩЕНИЯ
    string fullMessage(buffer);
    stringstream ss(fullMessage);
    string loginStr, passStr;
    ss >> loginStr;
    if (!ss.eof()) ss >> passStr;

    // 3.4 ПРОВЕРКА УЧЁТНЫХ ДАННЫХ
    int result = data->GetUserList()->CheckUser(loginStr, passStr);
    DWORD replyValue = 0;

    if (result == -1) {
        // Логин заблокирован - режим защиты
        Log(data->GetGui(), "[PROTECT] Ignored: " + loginStr);
        Sleep(ServerConfig::RETRY_DELAY_MS);  // 1000 мс задержка
        replyValue = 0;
    }
    else if (result == 1) {
        // Успешная авторизация
        Log(data->GetGui(), "[SUCCESS] Login: " + loginStr);
        replyValue = 1;
    }
    else {
        // Неверный пароль
        replyValue = 0;
    }

    // 3.5 АСИНХРОННАЯ ОТПРАВКА ОТВЕТА
    ResetEvent(data->GetOverlap()->hEvent);
    success = WriteFile(data->GetPipe(), &replyValue, sizeof(DWORD), 
                        &bytesTransferred, data->GetOverlap());
    
    if (!success && GetLastError() == ERROR_IO_PENDING) {
        GetOverlappedResult(data->GetPipe(), data->GetOverlap(), 
                            &bytesTransferred, TRUE);
    }
    
    FlushFileBuffers(data->GetPipe());  // Принудительная отправка
}
else {
    // 3.6 ОБРАБОТКА ОШИБОК
    DWORD error = GetLastError();
    
    if (error == ERROR_BROKEN_PIPE) {
        Log(data->GetGui(), "[INFO] Client disconnected during read");
    }
    else if (error == ERROR_NO_DATA) {
        Log(data->GetGui(), "[INFO] Pipe closing");
    }
    else if (error != 0) {
        char errorMsg[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
                      0, errorMsg, sizeof(errorMsg), NULL);
        Log(data->GetGui(), "[ERROR] (" + to_string(error) + "): " + errorMsg);
    }
}

// 3.7 ОТКЛЮЧЕНИЕ КЛИЕНТА
DisconnectNamedPipe(data->GetPipe());
// → возврат к началу цикла (ожидание нового подключения)
```

### 4.6 Диаграмма обработки запроса

```
┌──────────────────────────────────────────────────────────────────────┐
│                   ОБРАБОТКА ЗАПРОСА КЛИЕНТА                          │
└───────────────────────────────┬──────────────────────────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  ConnectNamedPipe()   │
                    │  (асинхронно)         │
                    └───────────┬───────────┘
                                │
                    ┌───────────┴───────────┐
                    │ERROR_IO_PENDING?      │
                    └───────────┬───────────┘
                                │ YES
                                ▼
                    ┌───────────────────────┐
                    │GetOverlappedResult()  │
                    │  (ждём подключения)   │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   ReadFile()          │
                    │   (асинхронно)        │
                    │   "login password"    │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   Парсинг сообщения   │
                    │   login = "david"     │
                    │   pass = "qwe"        │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  CheckUser(login,pass)│
                    └───────────┬───────────┘
                                │
            ┌───────────────────┼───────────────────┐
            │                   │                   │
            ▼                   ▼                   ▼
    ┌───────────────┐   ┌───────────────┐   ┌───────────────┐
    │   result=-1   │   │   result=0    │   │   result=1    │
    │  ЗАБЛОКИРОВАН │   │ НЕВЕРНЫЙ ПАРОЛЬ│   │    УСПЕХ     │
    └───────┬───────┘   └───────┬───────┘   └───────┬───────┘
            │                   │                   │
            ▼                   ▼                   ▼
    ┌───────────────┐   ┌───────────────┐   ┌───────────────┐
    │  Sleep(1s)    │   │  reply = 0    │   │  reply = 1    │
    │  reply = 0    │   │               │   │  Log SUCCESS  │
    │  Log PROTECT  │   │               │   │               │
    └───────┬───────┘   └───────┬───────┘   └───────┬───────┘
            │                   │                   │
            └───────────────────┼───────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   WriteFile()         │
                    │   (асинхронно)        │
                    │   → DWORD reply       │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  DisconnectNamedPipe()│
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │  Возврат к ожиданию   │
                    │  нового клиента       │
                    └───────────────────────┘
```

### 4.7 Логирование (метод `Log`)

**Потокобезопасная отправка логов в GUI:**

```cpp
static void Log(HWND hGui, const string& text) {
    // SendMessage - синхронный вызов, можно использовать локальный буфер
    // PostMessage требовал бы динамической аллокации (утечка памяти)
    SendMessage(hGui, WM_LOG_MSG, (WPARAM)text.c_str(), (LPARAM)text.length());
}
```

**Кастомное сообщение:**
```cpp
#define WM_LOG_MSG (WM_USER + 10)
```

---

## 5. Система противодействия взлому

### 5.1 Конфигурация (файл `ServerConstants.h`)

```cpp
namespace ServerConfig {
    const int PIPE_BUFFER_SIZE = 512;           // Размер буфера канала
    
    // Anti-Brute-Force защита
    const unsigned long BLOCK_DURATION_MS = 3000;  // Время блокировки: 3 секунды
    const unsigned long RETRY_DELAY_MS = 1000;     // Задержка перед повтором: 1 секунда
    
    // Управление потоками
    const unsigned long SHUTDOWN_TIMEOUT_MS = 5000; // Таймаут завершения: 5 секунд
}
```

### 5.2 Механизм блокировки логинов

**Принцип работы:**

```
┌─────────────────────────────────────────────────────────────────────┐
│                    МЕХАНИЗМ ANTI-BRUTE-FORCE                        │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────┐
│  1. При НЕВЕРНОМ ПАРОЛЕ:                                          │
│     blockedUsers[login] = GetTickCount() + 3000ms                 │
│     → логин блокируется на 3 секунды                              │
└───────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────┐
│  2. При следующем запросе с этим логином:                         │
│     if (currentTime < blockedUsers[login]) {                      │
│         return -1;  // ИГНОРИРУЕМ                                 │
│         Sleep(1000ms);  // Дополнительная задержка                │
│     }                                                             │
└───────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────┐
│  3. По истечении времени блокировки:                              │
│     blockedUsers.erase(login);  // Удаляем из списка              │
│     → запросы обрабатываются нормально                            │
└───────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────┐
│  4. При УСПЕШНОЙ авторизации:                                     │
│     blockedUsers.erase(login);  // Снимаем блокировку             │
└───────────────────────────────────────────────────────────────────┘
```

### 5.3 Влияние на производительность атаки

**Без защиты:**
```
Запрос 1 → 0 мс → Ответ
Запрос 2 → 0 мс → Ответ
Запрос 3 → 0 мс → Ответ
...
→ ~1000-5000 паролей/сек
```

**С защитой:**
```
Запрос 1 (неверный пароль) → 0 мс → Ответ 0
↓ логин блокируется на 3 сек
Запрос 2 → return -1 + Sleep(1s) → Ответ 0
Запрос 3 → return -1 + Sleep(1s) → Ответ 0
Запрос 4 → return -1 + Sleep(1s) → Ответ 0
↓ через 3 сек блокировка снимается
Запрос 5 (неверный пароль) → 0 мс → Ответ 0
↓ снова блокировка
...
→ ~0.3-1 пароль/сек на логин
```

### 5.4 Важные особенности защиты

1. **Блокировка на уровне логина** - разные логины не влияют друг на друга
2. **Потокобезопасность** - CRITICAL_SECTION защищает map блокировок
3. **Не блокирует сервер** - другие экземпляры каналов продолжают работать
4. **Временная блокировка** - автоматическое восстановление через N секунд
5. **Дополнительная задержка** - Sleep() увеличивает время отклика

---

## 6. Протокол обмена данными

### 6.1 Формат сообщения клиента

```
"login password"
```

- Логин и пароль разделены пробелом
- Строка завершается null-терминатором
- Максимальная длина: BUFFER_SIZE - 1 = 511 байт

### 6.2 Формат ответа сервера

```
DWORD (4 байта):
  0 = Неверный пароль / пользователь не найден / заблокирован
  1 = Успешная авторизация
```

### 6.3 Диаграмма последовательности

```
┌──────────┐                              ┌──────────┐
│  Client  │                              │  Server  │
└────┬─────┘                              └────┬─────┘
     │                                         │
     │  CreateFile("\\.\pipe\AuthPipe")        │
     │────────────────────────────────────────>│
     │                                         │
     │                ConnectNamedPipe()       │
     │<────────────────────────────────────────│
     │                                         │
     │  WriteFile("david qwe")                 │
     │────────────────────────────────────────>│
     │                                         │
     │              [CheckUser()]              │
     │                                         │
     │  ReadFile() ← DWORD: 1                  │
     │<────────────────────────────────────────│
     │                                         │
     │  CloseHandle()                          │
     │────────────────────────────────────────>│
     │                                         │
     │              DisconnectNamedPipe()      │
     │                                         │
```

### 6.4 Параметры Named Pipe

| Параметр | Значение | Описание |
|----------|----------|----------|
| **Имя** | `\\.\pipe\AuthPipe` | Локальный канал |
| **Режим доступа** | `PIPE_ACCESS_DUPLEX` | Двунаправленный |
| **Флаги** | `FILE_FLAG_OVERLAPPED` | Асинхронный режим |
| **Тип** | `PIPE_TYPE_MESSAGE` | Режим сообщений |
| **Режим чтения** | `PIPE_READMODE_MESSAGE` | Чтение сообщений |
| **Количество экземпляров** | `PIPE_UNLIMITED_INSTANCES` | Неограниченно |
| **Размер буфера** | 512 байт | Input и Output |

---

## 7. Соответствие требованиям

### 7.1 Основные требования к серверу

| Требование | Статус | Реализация |
|------------|--------|------------|
| **Запуск: запрос файла через GetOpenFileName** | ✅ | `UserList::Load()` - диалог выбора файла |
| **Формат файла: длины + пары логин/пароль** | ✅ | `LoadFromFile()` - парсинг формата |
| **Выбор режима работы** | ✅ | Чекбокс "Anti-Brute-Force Mode" |
| **Минимум 3 экземпляра канала** | ✅ | `Start(hwThreads * 2)` - динамическое количество |
| **Асинхронный режим** | ✅ | `FILE_FLAG_OVERLAPPED`, `OVERLAPPED` структура |
| **Ожидание подключения клиентов** | ✅ | `ConnectNamedPipe()` в цикле |
| **Проверка логина/пароля** | ✅ | `CheckUser()` → 1 или 0 |
| **Отключение клиента** | ✅ | `DisconnectNamedPipe()` |

### 7.2 Дополнительные требования

| Требование | Статус | Реализация |
|------------|--------|------------|
| **Работа по сети** | ✅ | Формат имени `\\COMPUTER\pipe\AuthPipe` |
| **Режим защиты от взлома** | ✅ | `protectionMode`, `blockedUsers` map |
| **Пропуск обработки запросов** | ✅ | `return -1` при блокировке + `Sleep()` |
| **Блокировка по логину** | ✅ | `map<string, DWORD> blockedUsers` |
| **Не влияет на скорость сервера** | ✅ | Отдельный map для каждого логина |

### 7.3 Требования к структуре файлов

| Требуемый файл | Реализация | Содержимое |
|----------------|------------|------------|
| `list.h` | ✅ `list.h` | Класс `UserList` для работы со списком пользователей |
| `PipeServer.h` | ✅ `PipeServer.h` | Класс `PipeServer` (асинхронный режим) |
| `PerPipeStruct.h` | ✅ `PerPipeStruct.h` | Класс `PerPipeData` для данных каждого канала |
| `test.cpp` | ✅ `test.cpp` | Функция `WinMain` + `WindowProc` |
| `ServerConstants.h` | ✅ (доп.) | Константы конфигурации |
| `ServerContext.h` | ✅ (доп.) | Инкапсуляция состояния сервера |

### 7.4 Технические требования

| Требование | Статус | Реализация |
|------------|--------|------------|
| **ООП обязательно** | ✅ | Классы: `PipeServer`, `UserList`, `PerPipeData`, `ServerContext` |
| **Инкапсуляция** | ✅ | Private поля + геттеры/сеттеры |
| **Передача через аргументы** | ✅ | `Start(numPipes, users, hGui)` |
| **Вывод с пояснениями** | ✅ | Логирование: `[SUCCESS]`, `[PROTECT]`, `[INFO]`, `[ERROR]` |

### 7.5 Требования к асинхронному режиму (из документации WinAPI)

| Требование | Статус | Реализация |
|------------|--------|------------|
| **OVERLAPPED с ручным сбросом** | ✅ | `CreateEvent(NULL, TRUE, FALSE, NULL)` |
| **Обнуление полей перед операцией** | ✅ | `ZeroMemory(buffer)`, `ResetEvent()` |
| **Не изменять Offset вручную** | ✅ | Не модифицируется |
| **Автоматическое изменение состояния** | ✅ | Система управляет событиями |

---

## Приложение А: Диаграмма полного жизненного цикла сервера

```
┌─────────────────────────────────────────────────────────────────────┐
│                    ПОЛНЫЙ ЖИЗНЕННЫЙ ЦИКЛ СЕРВЕРА                    │
└───────────────────────────────┬─────────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────────┐
│                          WinMain()                                    │
│ ┌───────────────────────────────────────────────────────────────────┐ │
│ │  1. RegisterClass()                                               │ │
│ │  2. CreateWindow("Password Server (Admin)")                       │ │
│ │  3. new ServerContext()                                           │ │
│ │  4. SetWindowLongPtr(GWLP_USERDATA, context)                      │ │
│ │  5. ShowWindow()                                                  │ │
│ │  6. Message Loop: GetMessage() → TranslateMessage() → Dispatch()  │ │
│ └───────────────────────────────────────────────────────────────────┘ │
└───────────────────────────────┬───────────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────────┐
│                       WM_CREATE                                       │
│ ┌───────────────────────────────────────────────────────────────────┐ │
│ │  CreateWindow("Load Users File")    → IDC_LOAD_BTN               │ │
│ │  CreateWindow("Anti-Brute-Force")   → IDC_PROTECT_CHK            │ │
│ │  CreateWindow("Start Server")       → IDC_START_BTN              │ │
│ │  CreateWindowEx("LISTBOX")          → IDC_LOG_LIST               │ │
│ └───────────────────────────────────────────────────────────────────┘ │
└───────────────────────────────┬───────────────────────────────────────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         │                      │                      │
         ▼                      ▼                      ▼
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────┐
│ IDC_LOAD_BTN    │  │ IDC_PROTECT_CHK │  │ IDC_START_BTN       │
│ ───────────────│  │ ─────────────── │  │ ─────────────────── │
│ GetOpenFileName │  │ SetProtection() │  │ Start(N, users, gui)│
│ LoadFromFile()  │  │ Log("ON"/"OFF") │  │ for N: CreateThread │
│ Log("Loaded N") │  │                 │  │ Log("Server ON")    │
└─────────────────┘  └─────────────────┘  └──────────┬──────────┘
                                                     │
                                                     ▼
                    ┌─────────────────────────────────────────────────┐
                    │              РАБОЧИЕ ПОТОКИ                     │
                    │ ┌─────────────────────────────────────────────┐ │
                    │ │  while (!shutdown) {                        │ │
                    │ │      ConnectNamedPipe() // асинхронно       │ │
                    │ │      ReadFile()         // асинхронно       │ │
                    │ │      CheckUser()                            │ │
                    │ │      WriteFile()        // асинхронно       │ │
                    │ │      DisconnectNamedPipe()                  │ │
                    │ │      SendMessage(WM_LOG_MSG)                │ │
                    │ │  }                                          │ │
                    │ └─────────────────────────────────────────────┘ │
                    └────────────────────────┬────────────────────────┘
                                             │
                                             ▼
                    ┌─────────────────────────────────────────────────┐
                    │                  WM_DESTROY                     │
                    │ ┌─────────────────────────────────────────────┐ │
                    │ │  1. SetEvent(hShutdownEvent)                │ │
                    │ │  2. WaitForMultipleObjects(threads)         │ │
                    │ │  3. CloseHandle(threads)                    │ │
                    │ │  4. delete context                          │ │
                    │ │  5. PostQuitMessage(0)                      │ │
                    │ └─────────────────────────────────────────────┘ │
                    └─────────────────────────────────────────────────┘
```

---

## Приложение Б: Тестовые данные (passwords.txt)

```
20 20                          # Макс. длина логина и пароля
david qwe                      # Тестовый пользователь (известен хакеру)
amanda 123456                  # Нарушение: общеизвестный пароль
michael password               # Нарушение: словарный пароль
williams abcdef                # Нарушение: простая последовательность
richard anna2000               # Нарушение: личные данные
john ivan1995                  # Нарушение: личные данные
jessica maksym                 # Имя
miller olga88                  # Имя + цифры
davis 12345                    # Нарушение: числовая последовательность
laura qweasd                   # Клавиатурный паттерн
garcia abc123                  # Нарушение: общеизвестный
hernandez pass123              # Нарушение: общеизвестный
jones mypass                   # Простой пароль
jessica_hernandez welcome      # Нарушение: словарный
david_williams letmein         # Нарушение: общеизвестный
anna admin                     # Нарушение: общеизвестный
william sunshine               # Нарушение: словарный
johnson dragon                 # Нарушение: словарный
brown monkey                   # Нарушение: словарный
anna_williams iloveyou         # Нарушение: общеизвестный
robert master                  # Нарушение: словарный
jessica_williams princess      # Нарушение: словарный
alice charlie                  # Нарушение: словарный
robert_williams shadow         # Нарушение: словарный
michael_brown football         # Нарушение: словарный
rodriguez baseball             # Нарушение: словарный
lopez batman                   # Нарушение: словарный
james superman                 # Нарушение: словарный
william_miller trustno1        # Нарушение: общеизвестный
amanda_williams starwars       # Нарушение: словарный
james_davis michael1           # Имя + цифра
anderson jessica1              # Имя + цифра
```

---

## Приложение В: Сравнение производительности

### Скорость обработки запросов

| Режим | Описание | Примерная скорость |
|-------|----------|-------------------|
| Без защиты | Нормальная работа | 1000-5000 паролей/сек |
| С защитой | После 1-й неудачной попытки | ~0.3 пароля/сек на логин |

### Время взлома (оценка)

| Сложность пароля | Без защиты | С защитой |
|------------------|------------|-----------|
| Простой (3 символа, 27 алфавит) | ~4 сек | ~16 часов |
| Средний (5 символов, 63 алфавит) | ~3 часа | ~1 год |
| Сложный (7+ символов) | Не реально | Не реально |

---

*Документ создан на основе анализа исходного кода серверного приложения.*
*Версия: 1.0*
*Дата: Декабрь 2024*
