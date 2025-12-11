#pragma once

#include <string>
#include <chrono>
#include <iostream>
#include <windows.h>

class Timer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    bool isRunning;

public:
    Timer();
    void Start();
    unsigned long long Stop();
    unsigned long long GetElapsed() const;
    void Reset();
    bool IsRunning() const { return isRunning; }
    std::string GetElapsedFormatted() const;
};

class AttackStats {
private:
    unsigned long long totalAttempts;
    unsigned long long startTime;
    unsigned long long endTime;

public:
    AttackStats();
    void Start();
    void Stop();
    void RecordAttempt();
    void AddAttempts(unsigned long long count);
    unsigned long long GetTotalAttempts() const { return totalAttempts; }
    unsigned long long GetElapsedMs() const;
    double GetAttemptsPerSecond() const;
    std::string GetStats() const;
};

// Windows error message helper
std::string GetWin32ErrorMessage(DWORD errorCode);

namespace Console {
    void ClearLine();
    void PrintProgressBar(double percent, unsigned long long current, unsigned long long total, int barWidth = 25);
    void PrintHeader(const std::string& title);
    void PrintSeparator(char ch = '=', int width = 60);

    enum Color {
        BLACK = 0,
        DARK_BLUE = 1,
        DARK_GREEN = 2,
        DARK_CYAN = 3,
        DARK_RED = 4,
        DARK_MAGENTA = 5,
        DARK_YELLOW = 6,
        LIGHT_GRAY = 7,
        DARK_GRAY = 8,
        LIGHT_BLUE = 9,
        LIGHT_GREEN = 10,
        LIGHT_CYAN = 11,
        LIGHT_RED = 12,
        LIGHT_MAGENTA = 13,
        LIGHT_YELLOW = 14,
        WHITE = 15
    };

    void SetColor(Color foreground, Color background = BLACK);
    void ResetColor();
    void PrintColored(Color color, const std::string& message);
    void PrintSuccess(const std::string& message);
    void PrintError(const std::string& message);
    void PrintInfo(const std::string& message);
    void PrintWarning(const std::string& message);
}
