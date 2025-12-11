#pragma once
#include <windows.h>
#include "list.h"

// Повідомлення для GUI
#define WM_LOG_MSG (WM_USER + 10)

// OOP-compliant structure for pipe thread context
class PerPipeData {
private:
    HANDLE hPipe;       // Хендл каналу
    int pipeId;         // Номер каналу
    HWND hGui;          // Хендл вікна
    UserList* userList; // База даних
    OVERLAPPED oOverlap; // Структура для асинхронних операцій
    HANDLE hShutdownEvent; // Event для graceful shutdown

public:
    PerPipeData() : hPipe(INVALID_HANDLE_VALUE), pipeId(0), hGui(NULL), userList(nullptr), hShutdownEvent(NULL) {
        ZeroMemory(&oOverlap, sizeof(OVERLAPPED));
    }
    
    PerPipeData(int id, HWND gui, UserList* users, HANDLE shutdownEvent) 
        : hPipe(INVALID_HANDLE_VALUE), pipeId(id), hGui(gui), userList(users), hShutdownEvent(shutdownEvent) {
        ZeroMemory(&oOverlap, sizeof(OVERLAPPED));
    }
    
    // Getters
    HANDLE GetPipe() const { return hPipe; }
    int GetPipeId() const { return pipeId; }
    HWND GetGui() const { return hGui; }
    UserList* GetUserList() const { return userList; }
    OVERLAPPED* GetOverlap() { return &oOverlap; }
    HANDLE GetShutdownEvent() const { return hShutdownEvent; }
    
    // Setters
    void SetPipe(HANDLE pipe) { hPipe = pipe; }
    void SetPipeId(int id) { pipeId = id; }
    void SetGui(HWND gui) { hGui = gui; }
    void SetUserList(UserList* users) { userList = users; }
    void SetShutdownEvent(HANDLE event) { hShutdownEvent = event; }
    
    // For backward compatibility - direct access (to minimize changes)
    HANDLE& PipeRef() { return hPipe; }
    OVERLAPPED& OverlapRef() { return oOverlap; }
};