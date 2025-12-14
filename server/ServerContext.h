#pragma once

#include "list.h"
#include "PipeServer.h"

// Клас контексту для інкапсуляції стану сервера
// Замінює глобальні змінні правильною інкапсуляцією
class ServerContext {
private:
    UserList users;
    PipeServer server;

public:
    ServerContext() = default;

    // Заборона копіювання
    ServerContext(const ServerContext&) = delete;
    ServerContext& operator=(const ServerContext&) = delete;

    // Доступ до списку користувачів
    UserList& GetUsers() {
        return users;
    }

    const UserList& GetUsers() const {
        return users;
    }

    // Доступ до сервера
    PipeServer& GetServer() {
        return server;
    }

    const PipeServer& GetServer() const {
        return server;
    }
};