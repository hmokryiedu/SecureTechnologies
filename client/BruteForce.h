#pragma once

#include <string>

class BruteForceGenerator {
private:
    std::string alphabet;
    int maxLength;
    std::string currentPassword;
    unsigned long long attemptCount;
    unsigned long long totalCombinations;

public:
    BruteForceGenerator(const std::string& charset, int maxLen);
    bool HasNext() const;
    std::string Next();
    void Reset();
    std::string GetCurrent() const { return currentPassword; }
    unsigned long long GetAttemptCount() const { return attemptCount; }
    unsigned long long GetTotalCombinations() const { return totalCombinations; }
    double GetProgressPercent() const;
    int GetCurrentLength() const { return currentPassword.length(); }

private:
    unsigned long long CalculateTotalCombinations(int alphabetSize, int maxLen);
    bool IncrementPassword();
};
