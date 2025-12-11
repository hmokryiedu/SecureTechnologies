#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

// Клас для роботи зі списком користувачів
class UserList {
private:
    struct User {
        string login;
        string password;
    };

    vector<User> users;
    map<string, DWORD> blockedUsers; // Логін -> Час розблокування (GetTickCount)
    bool protectionMode; // Режим захисту
    int maxLoginLen;
    int maxPassLen;
    
    CRITICAL_SECTION blockedUsersLock; // Thread synchronization for blockedUsers map

public:
    UserList() : protectionMode(false), maxLoginLen(0), maxPassLen(0) {
        InitializeCriticalSection(&blockedUsersLock);
    }
    
    ~UserList() {
        DeleteCriticalSection(&blockedUsersLock);
    }

    void SetProtection(bool enable) {
        protectionMode = enable;
    }

    // Завантаження файлу через діалогове вікно
    bool Load(HWND hwnd) {
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
            ifstream file(ofn.lpstrFile);
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
        return false;
    }

    size_t Count() const { return users.size(); }

    // Перевірка: 1 = ОК, 0 = Невірний пароль, -1 = Заблоковано
    int CheckUser(const string& login, const string& pass) {
        // 1. Перевірка блокування (Anti-Brute-Force) - thread-safe
        if (protectionMode) {
            EnterCriticalSection(&blockedUsersLock);
            
            DWORD currentTime = GetTickCount();
            if (blockedUsers.count(login)) {
                if (currentTime < blockedUsers[login]) {
                    LeaveCriticalSection(&blockedUsersLock);
                    return -1; // Ігноруємо запит
                } else {
                    blockedUsers.erase(login); // Час бану вийшов
                }
            }
            
            LeaveCriticalSection(&blockedUsersLock);
        }

        // 2. Пошук користувача
        for (const auto& u : users) {
            if (u.login == login) {
                if (u.password == pass) {
                    if (protectionMode) {
                        EnterCriticalSection(&blockedUsersLock);
                        blockedUsers.erase(login);
                        LeaveCriticalSection(&blockedUsersLock);
                    }
                    return 1; // Успіх
                } else {
                    // Невірний пароль
                    if (protectionMode) {
                        EnterCriticalSection(&blockedUsersLock);
                        blockedUsers[login] = GetTickCount() + 3000; // Бан на 3 секунди
                        LeaveCriticalSection(&blockedUsersLock);
                    }
                    return 0; // Пароль невірний
                }
            }
        }
        return 0; // Користувача не знайдено
    }
};