#pragma once

#include "list.h"
#include "PipeServer.h"

// Context class to encapsulate server state
// Replaces global variables with proper encapsulation
class ServerContext {
private:
    UserList users;
    PipeServer server;

public:
    ServerContext() = default;

    // Prevent copying
    ServerContext(const ServerContext&) = delete;
    ServerContext& operator=(const ServerContext&) = delete;

    // Access to user list
    UserList& GetUsers() {
        return users;
    }

    const UserList& GetUsers() const {
        return users;
    }

    // Access to server
    PipeServer& GetServer() {
        return server;
    }

    const PipeServer& GetServer() const {
        return server;
    }
};
