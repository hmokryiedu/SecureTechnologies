#pragma once
#include <windows.h>
#include "list.h"

// Структура, що передається в потік
struct PerPipeData {
    HANDLE hPipe;       // Хендл каналу
    int pipeId;         // Номер каналу (для логів)
    HWND hGui;          // Хендл вікна (щоб слати логи)
    UserList* userList; // Вказівник на базу даних
};

// Повідомлення для GUI
#define WM_LOG_MSG (WM_USER + 10)