#pragma once

#include <atomic>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

// Shared state between worker threads
struct AttackContext {
    std::atomic<bool> passwordFound;
    std::atomic<unsigned long long> totalAttempts;

    std::mutex resultMutex;
    std::string foundPassword;

    const std::string targetLogin;

    explicit AttackContext(const std::string& login)
        : passwordFound(false), totalAttempts(0), targetLogin(login) {}

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

    void IncrementAttempts() {
        totalAttempts.fetch_add(1);
    }

    unsigned long long GetAttempts() const {
        return totalAttempts.load();
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
