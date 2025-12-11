#pragma once

#include <atomic>
#include <mutex>
#include <string>

// Context class to encapsulate shared state for multithreaded attacks
// Replaces global variables with proper encapsulation
class AttackContext {
private:
    std::atomic<bool> passwordFound;
    std::atomic<unsigned long long> totalAttempts;
    std::mutex consoleMutex;
    std::string foundPassword;
    std::mutex foundPasswordMutex;

public:
    AttackContext() : passwordFound(false), totalAttempts(0), foundPassword("") {}

    // Thread-safe password found flag
    bool IsPasswordFound() const {
        return passwordFound.load();
    }

    void SetPasswordFound(bool found) {
        passwordFound.store(found);
    }

    // Thread-safe attempt counter
    unsigned long long GetTotalAttempts() const {
        return totalAttempts.load();
    }

    void IncrementAttempts() {
        totalAttempts.fetch_add(1);
    }

    // Thread-safe found password access
    std::string GetFoundPassword() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(foundPasswordMutex));
        return foundPassword;
    }

    void SetFoundPassword(const std::string& password) {
        std::lock_guard<std::mutex> lock(foundPasswordMutex);
        foundPassword = password;
    }

    // Console synchronization
    std::mutex& GetConsoleMutex() {
        return consoleMutex;
    }

    // Reset state for new attack
    void Reset() {
        passwordFound.store(false);
        totalAttempts.store(0);
        std::lock_guard<std::mutex> lock(foundPasswordMutex);
        foundPassword.clear();
    }
};
