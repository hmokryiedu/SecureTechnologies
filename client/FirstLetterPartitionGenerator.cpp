#include "FirstLetterPartitionGenerator.h"
#include <cmath>

FirstLetterPartitionGenerator::FirstLetterPartitionGenerator(
    const std::string& fullAlphabet,
    int maxLen,
    const std::vector<char>& firstLetters)
    : alphabet(fullAlphabet),
      myFirstLetters(firstLetters),
      maxLength(maxLen),
      currentFirstLetterIdx(0),
      attemptCount(0),
      finished(false)
{
    if (myFirstLetters.empty()) {
        finished = true;
    } else {
        // Initialize with first password: myFirstLetters[0]
        currentPassword = std::string(1, myFirstLetters[0]);
    }
}

bool FirstLetterPartitionGenerator::HasNext() const {
    return !finished;
}

std::string FirstLetterPartitionGenerator::Next() {
    if (finished) return "";

    std::string result = currentPassword;
    attemptCount++;

    Increment();

    return result;
}

void FirstLetterPartitionGenerator::Increment() {
    // Single char password -> add first suffix char
    if (currentPassword.length() == 1) {
        if (maxLength > 1) {
            currentPassword += alphabet[0];
        } else {
            // maxLength is 1, move to next first letter
            currentFirstLetterIdx++;
            if (currentFirstLetterIdx >= myFirstLetters.size()) {
                finished = true;
            } else {
                currentPassword = std::string(1, myFirstLetters[currentFirstLetterIdx]);
            }
        }
        return;
    }

    // Try incrementing suffix from rightmost position
    int pos = currentPassword.length() - 1;

    while (pos > 0) {  // Don't touch position 0 (first letter)
        size_t charIdx = alphabet.find(currentPassword[pos]);

        if (charIdx != std::string::npos && charIdx < alphabet.length() - 1) {
            // Can increment this position
            currentPassword[pos] = alphabet[charIdx + 1];
            return;
        }

        // Overflow - reset to 0 and carry left
        currentPassword[pos] = alphabet[0];
        pos--;
    }

    // All suffix positions overflowed
    if (currentPassword.length() < maxLength) {
        // Increase length: add new position
        currentPassword += alphabet[0];
    } else {
        // Max length reached - move to next first letter
        currentFirstLetterIdx++;

        if (currentFirstLetterIdx >= myFirstLetters.size()) {
            finished = true;
        } else {
            // Reset to new first letter, length 1
            currentPassword = std::string(1, myFirstLetters[currentFirstLetterIdx]);
        }
    }
}

void FirstLetterPartitionGenerator::Reset() {
    currentFirstLetterIdx = 0;
    attemptCount = 0;
    finished = myFirstLetters.empty();

    if (!finished) {
        currentPassword = std::string(1, myFirstLetters[0]);
    }
}

double FirstLetterPartitionGenerator::GetProgressPercent() const {
    unsigned long long total = CalculateTotalCombinations();
    if (total == 0) return 0.0;
    return (double)attemptCount / (double)total * 100.0;
}

size_t FirstLetterPartitionGenerator::GetTotalCount() const {
    return static_cast<size_t>(CalculateTotalCombinations());
}

unsigned long long FirstLetterPartitionGenerator::CalculateTotalCombinations() const {
    if (myFirstLetters.empty()) return 0;

    unsigned long long total = 0;
    size_t alphabetSize = alphabet.length();

    // For each first letter assigned to this generator
    for (size_t i = 0; i < myFirstLetters.size(); i++) {
        // Length 1: just the first letter
        total += 1;

        // Lengths 2 to maxLength: first letter + (len-1) suffix chars
        unsigned long long suffixPower = alphabetSize;
        for (int len = 2; len <= maxLength; len++) {
            total += suffixPower;
            suffixPower *= alphabetSize;
        }
    }

    return total;
}
