#pragma once

#include <string>
#include "PasswordGenerator.h"

class BruteForceGenerator : public PasswordGenerator {
private:
    std::string alphabet;
    int maxLength;
    std::string currentPassword;
    unsigned long long attemptCount;
    unsigned long long totalCombinations;

public:
    BruteForceGenerator(const std::string& charset, int maxLen);
    
    // PasswordGenerator interface implementation
    bool HasNext() const override;
    std::string Next() override;
    void Reset() override;
    double GetProgressPercent() const override;
    size_t GetTotalCount() const override { return static_cast<size_t>(totalCombinations); }
    
    // Brute-force specific methods
    std::string GetCurrent() const { return currentPassword; }
    unsigned long long GetAttemptCount() const { return attemptCount; }
    unsigned long long GetTotalCombinations() const { return totalCombinations; }
    int GetCurrentLength() const { return currentPassword.length(); }
    
    // Get password at specific index (for multithreading)
    std::string GetPasswordAtIndex(unsigned long long index) const;

private:
    unsigned long long CalculateTotalCombinations(int alphabetSize, int maxLen);
    bool IncrementPassword();
};
