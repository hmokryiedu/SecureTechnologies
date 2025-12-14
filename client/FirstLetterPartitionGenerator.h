#pragma once

#include <string>
#include <vector>
#include "PasswordGenerator.h"

// Generates passwords starting with assigned first letters
// Uses full alphabet for remaining positions
class FirstLetterPartitionGenerator : public PasswordGenerator {
private:
    std::string alphabet;              // Full alphabet for positions 1+
    std::vector<char> myFirstLetters;  // Assigned first letters
    int maxLength;

    // Current state
    size_t currentFirstLetterIdx;      // Which first letter we're on
    std::string currentPassword;       // Current password being tested
    unsigned long long attemptCount;
    bool finished;

public:
    FirstLetterPartitionGenerator(const std::string& fullAlphabet,
                                  int maxLen,
                                  const std::vector<char>& firstLetters);

    // PasswordGenerator interface
    bool HasNext() const override;
    std::string Next() override;
    void Reset() override;
    double GetProgressPercent() const override;
    size_t GetTotalCount() const override;

    // Stats
    unsigned long long GetAttemptCount() const { return attemptCount; }

private:
    void Increment();  // Move to next password
    unsigned long long CalculateTotalCombinations() const;
};
