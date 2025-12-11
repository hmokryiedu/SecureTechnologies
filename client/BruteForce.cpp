#include "BruteForce.h"
#include <cmath>
#include <algorithm>

BruteForceGenerator::BruteForceGenerator(const std::string& charset, int maxLen)
    : alphabet(charset), maxLength(maxLen), currentPassword(""), attemptCount(0) {
    totalCombinations = CalculateTotalCombinations((int)alphabet.length(), maxLen);
}

bool BruteForceGenerator::HasNext() const {
    if (currentPassword.empty()) {
        return true;
    }
    if (currentPassword.length() == maxLength) {
        std::string maxPassword(maxLength, alphabet.back());
        return currentPassword != maxPassword;
    }
    return true;
}

std::string BruteForceGenerator::Next() {
    if (currentPassword.empty()) {
        currentPassword = std::string(1, alphabet[0]);
        attemptCount = 1;
        return currentPassword;
    }

    if (!IncrementPassword()) {
        currentPassword = "";
        return "";
    }

    attemptCount++;
    return currentPassword;
}

void BruteForceGenerator::Reset() {
    currentPassword = "";
    attemptCount = 0;
}

double BruteForceGenerator::GetProgressPercent() const {
    if (totalCombinations == 0) return 0.0;
    return (double)attemptCount / (double)totalCombinations * 100.0;
}

unsigned long long BruteForceGenerator::CalculateTotalCombinations(int alphabetSize, int maxLen) {
    if (alphabetSize <= 1) {
        return (unsigned long long)maxLen;
    }

    unsigned long long total = 0;
    unsigned long long power = alphabetSize;

    for (int i = 1; i <= maxLen; i++) {
        total += power;
        if (power > (unsigned long long)(-1) / alphabetSize) {
            return (unsigned long long)(-1);
        }
        power *= alphabetSize;
    }

    return total;
}

bool BruteForceGenerator::IncrementPassword() {
    if (currentPassword.empty()) {
        return false;
    }

    int pos = (int)currentPassword.length() - 1;

    while (pos >= 0) {
        size_t charIndex = alphabet.find(currentPassword[pos]);

        if (charIndex == std::string::npos) {
            currentPassword[pos] = alphabet[0];
            pos--;
            continue;
        }

        if (charIndex < alphabet.length() - 1) {
            currentPassword[pos] = alphabet[charIndex + 1];
            return true;
        }

        currentPassword[pos] = alphabet[0];
        pos--;
    }

    if (currentPassword.length() < (size_t)maxLength) {
        currentPassword = std::string(currentPassword.length() + 1, alphabet[0]);
        return true;
    }

    return false;
}

// Get password at specific index for multithreading
std::string BruteForceGenerator::GetPasswordAtIndex(unsigned long long index) const {
    if (index == 0 || index > totalCombinations) {
        return "";
    }

    int alphabetSize = (int)alphabet.length();
    unsigned long long currentIndex = 0;
    
    // Find which length group this index belongs to
    for (int len = 1; len <= maxLength; len++) {
        unsigned long long combosForThisLength = 1;
        for (int i = 0; i < len; i++) {
            combosForThisLength *= alphabetSize;
        }
        
        if (currentIndex + combosForThisLength >= index) {
            // This is the right length
            unsigned long long offsetInLength = index - currentIndex - 1;
            
            // Generate password of this length at this offset
            std::string result(len, alphabet[0]);
            
            for (int pos = len - 1; pos >= 0; pos--) {
                result[pos] = alphabet[offsetInLength % alphabetSize];
                offsetInLength /= alphabetSize;
            }
            
            return result;
        }
        
        currentIndex += combosForThisLength;
    }
    
    return "";
}
