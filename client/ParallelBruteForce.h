#pragma once

#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include "ClientConstants.h"

// Shared state between worker threads
struct AttackContext {
    std::atomic<bool> passwordFound;
    std::atomic<bool> timeoutReached;
    std::atomic<unsigned long long> totalAttempts;
    std::chrono::high_resolution_clock::time_point startTime;

    std::mutex resultMutex;
    std::string foundPassword;

    const std::string targetLogin;

    explicit AttackContext(const std::string& login)
        : passwordFound(false), timeoutReached(false), totalAttempts(0), 
          startTime(std::chrono::high_resolution_clock::now()), targetLogin(login) {}

    void MarkFound(const std::string& password) {
        std::lock_guard<std::mutex> lock(resultMutex);
        if (foundPassword.empty()) {  // First one wins
            foundPassword = password;
            passwordFound.store(true);
        }
    }

    bool IsFound() const {
        return passwordFound.load();
    }

    bool IsTimedOut() const {
        if (timeoutReached.load()) return true;
        
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        
        return elapsed >= static_cast<long long>(ClientConfig::PASSWORD_CRACKING_TIMEOUT_MS);
    }

    void CheckAndSetTimeout() {
        if (timeoutReached.load()) return;
        
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        
        if (elapsed >= static_cast<long long>(ClientConfig::PASSWORD_CRACKING_TIMEOUT_MS)) {
            timeoutReached.store(true);
        }
    }

    bool ShouldStop() {
        CheckAndSetTimeout();
        return IsFound() || IsTimedOut();
    }

    void IncrementAttempts() {
        totalAttempts.fetch_add(1);
    }

    unsigned long long GetAttempts() const {
        return totalAttempts.load();
    }

    unsigned long long GetElapsedMs() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    }
};

// Helper function to partition alphabet
std::vector<std::vector<char>> PartitionAlphabet(const std::string& alphabet, int threadCount);

// Worker thread function
void BruteForceWorker(int threadId,
                     const std::string& alphabet,
                     int maxLength,
                     const std::vector<char>& myFirstLetters,
                     AttackContext* context);
