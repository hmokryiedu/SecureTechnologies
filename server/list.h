#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include "ServerConstants.h"

using namespace std;

// Клас для роботи зі списком користувачів
class UserList {
private:
    struct User {
        string login;
        string password;
    };

    vector<User> users;
    
    // Мапа: Логін -> Кількість невдалих спроб
    // Ми використовуємо це замість часового бану
    map<string, int> attemptCounts; 
    
    bool protectionMode; // Режим захисту
    bool isServerRunning;
    int maxLoginLen;
    int maxPassLen;
    
    CRITICAL_SECTION cs; // Синхронізація потоків

public:
    UserList() : protectionMode(false), isServerRunning(false), maxLoginLen(0), maxPassLen(0) {
        InitializeCriticalSection(&cs);
    }
    
    ~UserList() {
        DeleteCriticalSection(&cs);
    }

    void SetProtection(bool enable) {
        protectionMode = enable;
        // При зміні режиму скидаємо лічильники, щоб почати з чистого аркуша
        EnterCriticalSection(&cs);
        attemptCounts.clear();
        LeaveCriticalSection(&cs);
    }
    
    void SetServerRunning(bool running) {
        isServerRunning = running;
    }
    
    bool IsServerRunning() const {
        return isServerRunning;
    }

    // Завантаження файлу через діалогове вікно
    bool Load(HWND hwnd) {
        if (isServerRunning) {
            MessageBoxA(hwnd, "Cannot load users while server is running!\nPlease stop the server first.", 
                       "Server Running", MB_ICONWARNING | MB_OK);
            return false;
        }
        
        OPENFILENAMEA ofn;
        char szFile[260] = { 0 };

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            return LoadFromFile(ofn.lpstrFile);
        }
        return false;
    }
    
    // Зчитування з файлу
    bool LoadFromFile(const string& filepath) {
        ifstream file(filepath);
        if (!file.is_open()) return false;

        users.clear();
        string line;

        // Читаємо довжини (перший рядок)
        if (getline(file, line)) {
            stringstream ss(line);
            ss >> maxLoginLen >> maxPassLen;
        }

        // Читаємо пари логін пароль
        while (getline(file, line)) {
            stringstream ss(line);
            string l, p;
            if (ss >> l >> p) {
                users.push_back({ l, p });
            }
        }
        return true;
    }

    size_t Count() const { return users.size(); }

    // === ГОЛОВНА ЛОГІКА ПЕРЕВІРКИ ===
    // Повертає: 1 (Успіх), 0 (Невірно)
    // Змінює: outNeedDelay (true, якщо треба зачекати 1 секунду)
    int CheckUser(const string& login, const string& pass, bool& outNeedDelay) {
        EnterCriticalSection(&cs);

        // 1. За замовчуванням затримка не потрібна
        outNeedDelay = false;

        // 2. Якщо захист увімкнено, перевіряємо лічильник спроб
        if (protectionMode) {
            // Якщо спроб >= 3, повідомляємо серверу, що треба ввімкнути затримку
            if (attemptCounts[login] >= ServerConfig::MAX_ATTEMPTS_BEFORE_DELAY) {
                outNeedDelay = true;
            }
        }

        int result = 0; // За замовчуванням "Невірно"

        // 3. Шукаємо користувача
        bool found = false;
        for (const auto& u : users) {
            if (u.login == login) {
                found = true;
                if (u.password == pass) {
                    // --- ПАРОЛЬ ВІРНИЙ ---
                    result = 1;
                    // Успішний вхід скидає лічильник помилок
                    attemptCounts[login] = 0; 
                } else {
                    // --- ПАРОЛЬ НЕВІРНИЙ ---
                    result = 0;
                    if (protectionMode) {
                        // Збільшуємо лічильник помилок
                        attemptCounts[login]++; 
                    }
                }
                break; // Користувача знайдено, виходимо з циклу
            }
        }
        
        LeaveCriticalSection(&cs);
        return result;
    }
};