#include "Utils.h"
#include <windows.h>
#include <iomanip>
#include <sstream>
#include <cmath>

// ============= Windows Error Helper =============

std::string GetWin32ErrorMessage(DWORD errorCode) {
    char* messageBuffer = nullptr;

    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer,
        0,
        NULL
    );

    std::string message;
    if (size > 0 && messageBuffer) {
        message = std::string(messageBuffer, size);
        // Remove trailing newlines
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
        LocalFree(messageBuffer);
    } else {
        message = "Unknown error " + std::to_string(errorCode);
    }

    return message;
}

// ============= Timer Implementation =============

Timer::Timer() : isRunning(false) {
    startTime = std::chrono::high_resolution_clock::now();
}

void Timer::Start() {
    startTime = std::chrono::high_resolution_clock::now();
    isRunning = true;
}

unsigned long long Timer::Stop() {
    isRunning = false;
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    return duration.count();
}

unsigned long long Timer::GetElapsed() const {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
    return duration.count();
}

void Timer::Reset() {
    startTime = std::chrono::high_resolution_clock::now();
    isRunning = false;
}

std::string Timer::GetElapsedFormatted() const {
    unsigned long long ms = GetElapsed();
    unsigned long long seconds = ms / 1000;
    unsigned long long minutes = seconds / 60;
    unsigned long long hours = minutes / 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h " << (minutes % 60) << "m " << (seconds % 60) << "s";
    } else if (minutes > 0) {
        oss << minutes << "m " << (seconds % 60) << "s";
    } else {
        oss << std::fixed << std::setprecision(1) << (ms / 1000.0) << "s";
    }
    return oss.str();
}

// ============= AttackStats Implementation =============

AttackStats::AttackStats() : totalAttempts(0), startTime(0), endTime(0) {
}

void AttackStats::Start() {
    startTime = GetTickCount64();
}

void AttackStats::Stop() {
    endTime = GetTickCount64();
}

void AttackStats::RecordAttempt() {
    totalAttempts++;
}

void AttackStats::AddAttempts(unsigned long long count) {
    totalAttempts += count;
}

unsigned long long AttackStats::GetElapsedMs() const {
    if (endTime > 0) {
        return endTime - startTime;
    }
    return GetTickCount64() - startTime;
}

double AttackStats::GetAttemptsPerSecond() const {
    unsigned long long elapsedMs = GetElapsedMs();
    if (elapsedMs == 0) return 0.0;
    return (totalAttempts * 1000.0) / elapsedMs;
}

std::string AttackStats::GetStats() const {
    unsigned long long elapsedMs = GetElapsedMs();
    unsigned long long seconds = elapsedMs / 1000;
    unsigned long long minutes = seconds / 60;
    unsigned long long hours = minutes / 60;

    std::ostringstream oss;
    oss << "Attempts: " << totalAttempts << " | ";
    oss << "Rate: " << std::fixed << std::setprecision(1) << GetAttemptsPerSecond() << " pwd/sec | ";
    oss << "Time: ";

    if (hours > 0) {
        oss << hours << "h " << (minutes % 60) << "m " << (seconds % 60) << "s";
    } else if (minutes > 0) {
        oss << minutes << "m " << (seconds % 60) << "s";
    } else {
        oss << seconds << "s";
    }

    return oss.str();
}

// ============= Console Utilities Implementation =============

void Console::ClearLine() {
    // Move to start of line and clear
    std::cout << "\r";
    for (int i = 0; i < 120; i++) {
        std::cout << " ";
    }
    std::cout << "\r";
}

void Console::PrintProgressBar(double percent, unsigned long long current, unsigned long long total, int barWidth) {
    if (percent < 0.0) percent = 0.0;
    if (percent > 100.0) percent = 100.0;

    int filledWidth = (int)(barWidth * percent / 100.0);

    std::cout << "\r[";
    for (int i = 0; i < barWidth; i++) {
        if (i < filledWidth) {
            std::cout << "█";
        } else {
            std::cout << "░";
        }
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percent << "% ("
              << current << "/" << total << ")";
    std::cout.flush();
}

void Console::PrintHeader(const std::string& title) {
    std::cout << "\n";
    PrintSeparator('=', 70);
    std::cout << "  " << title << "\n";
    PrintSeparator('=', 70);
}

void Console::PrintSeparator(char ch, int width) {
    for (int i = 0; i < width; i++) {
        std::cout << ch;
    }
    std::cout << "\n";
}

void Console::SetColor(Color foreground, Color background) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(handle, (background << 4) | foreground);
}

void Console::ResetColor() {
    SetColor(LIGHT_GRAY, BLACK);
}

void Console::PrintColored(Color color, const std::string& message) {
    SetColor(color);
    std::cout << message;
    ResetColor();
    std::cout << "\n";
}

void Console::PrintSuccess(const std::string& message) {
    PrintColored(LIGHT_GREEN, "[+] " + message);
}

void Console::PrintError(const std::string& message) {
    PrintColored(LIGHT_RED, "[!] " + message);
}

void Console::PrintInfo(const std::string& message) {
    PrintColored(LIGHT_CYAN, "[*] " + message);
}

void Console::PrintWarning(const std::string& message) {
    PrintColored(LIGHT_YELLOW, "[?] " + message);
}
