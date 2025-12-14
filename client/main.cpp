#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "PipeClient.h"
#include "BruteForce.h"
#include "RuleAttack.h"
#include "Utils.h"
#include "ClientConstants.h"
#include "ParallelBruteForce.h"

// Forward declarations
void ShowMainMenu();
void ModeConnectionTest();
void ModeBruteForce();
void ModeRuleBasedAttack();

// Pause and wait for Enter key
void Pause() {
    std::cout << "\nPress Enter to continue...";
    std::cin.ignore(10000, '\n');
}

int main() {
    int choice = 0;
    while (true) {
        ShowMainMenu();
        std::cout << "\nEnter your choice (0-3): ";
        std::cin >> choice;
        std::cin.ignore(10000, '\n');

        switch (choice) {
            case 1:
                ModeConnectionTest();
                break;
            case 2:
                ModeBruteForce();
                break;
            case 3:
                ModeRuleBasedAttack();
                break;
            case 0:
                Console::PrintInfo("Exiting...");
                return 0;
            default:
                Console::PrintError("Invalid choice!");
                continue;
        }

        Pause();
    }

    return 0;
}

// ============= Menu =============

void ShowMainMenu() {
    Console::PrintSeparator('=', 70);
    std::cout << " PASSWORD CRACKING CLIENT - Educational Version\n";
    Console::PrintSeparator('=', 70);
    std::cout << " 1. Connection Test       - Verify server connectivity\n";
    std::cout << " 2. Brute Force Attack    - Try all password combinations\n";
    std::cout << " 3. Dictionary Attack     - Rule-based dictionary attack\n";
    std::cout << " 0. Exit\n";
    Console::PrintSeparator('=', 70);
}

void ModeConnectionTest() {
    Console::PrintHeader("CONNECTION TEST");

    PipeClient client;

    Console::PrintInfo("Connecting to server at \\\\.\\pipe\\AuthPipe...");
    if (!client.Connect()) {
        Console::PrintError("Failed to connect to server!");
        return;
    }
    Console::PrintSuccess("Connected!");

    // Get credentials from user
    std::string login, password;
    std::cout << "Enter login: ";
    std::getline(std::cin, login);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    Console::PrintInfo("Attempting login: " + login);

    if (client.TryPassword(login, password)) {
        Console::PrintSuccess("PASSWORD CORRECT!");
    } else {
        Console::PrintError("Password incorrect!");
    }

    client.Disconnect();
    Console::PrintInfo("Disconnected from server");
}

// ============= Mode 2: Brute Force Attack =============

void ModeBruteForce() {
    Console::PrintHeader("BRUTE FORCE ATTACK");

    // Select alphabet
    int alphabetChoice = 0;
    std::cout << "\nSelect alphabet:\n";
    std::cout << " 1. Lowercase + '      (27 characters)\n";
    std::cout << " 2. Letters + Digits + ' (63 characters)\n";
    std::cout << " 3. Full Alphabet      (128 characters: A-Z, a-z, 0-9, А-Я, а-я, ')\n";
    std::cout << "Enter choice (1-3): ";
    std::cin >> alphabetChoice;
    std::cin.ignore(10000, '\n');

    std::string alphabet;
    switch (alphabetChoice) {
        case 1:
            alphabet = "abcdefghijklmnopqrstuvwxyz'";
            break;
        case 2:
            alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'";
            break;
        case 3:
            // Full alphabet with Cyrillic
            alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
                      "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюя'";
            break;
        default:
            Console::PrintError("Invalid choice!");
            return;
    }

    // Get maximum password length
    int maxLength = 0;
    std::cout << "Enter maximum password length (1-20): ";
    std::cin >> maxLength;
    std::cin.ignore(10000, '\n');

    if (maxLength < ClientConfig::MIN_PASSWORD_LENGTH || maxLength > ClientConfig::MAX_PASSWORD_LENGTH) {
        Console::PrintError("Invalid length! Must be between " +
                          std::to_string(ClientConfig::MIN_PASSWORD_LENGTH) + " and " +
                          std::to_string(ClientConfig::MAX_PASSWORD_LENGTH) + " characters.");
        return;
    }

    // Get login to crack
    std::string targetLogin;
    std::cout << "Enter login to crack: ";
    std::getline(std::cin, targetLogin);

    // Auto-detect and configure thread count
    unsigned int hwThreads = std::thread::hardware_concurrency();
    int maxAllowed = std::min((int)hwThreads, 16);  // Cap at server's 16 pipes

    int threadCount = maxAllowed;
    std::cout << "\nHardware threads detected: " << hwThreads << "\n";
    std::cout << "Using " << threadCount << " threads (0 to auto-detect, 1-" << maxAllowed << " to specify): ";
    std::cin >> threadCount;
    std::cin.ignore(10000, '\n');

    if (threadCount == 0) threadCount = maxAllowed;
    if (threadCount < 1) threadCount = 1;
    if (threadCount > maxAllowed) threadCount = maxAllowed;

    Console::PrintInfo("Connecting to server...");

    // Create shared attack context
    AttackContext context(targetLogin);

    // Partition alphabet among threads
    std::vector<std::vector<char>> partitions = PartitionAlphabet(alphabet, threadCount);

    Console::PrintInfo("Starting brute force attack with " + std::to_string(threadCount) + " threads...");
    std::cout << "Target: " << targetLogin << "\n";
    std::cout << "Alphabet size: " << alphabet.length() << " characters\n";
    std::cout << "Max length: " << maxLength << "\n\n";

    // Launch worker threads
    std::vector<std::thread> workers;
    Timer timer;
    timer.Start();

    for (int i = 0; i < threadCount; i++) {
        workers.emplace_back(BruteForceWorker, i, alphabet, maxLength, partitions[i], &context);
    }

    // Progress monitoring loop
    while (!context.IsFound()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Check if all threads finished
        bool allFinished = true;
        for (auto& t : workers) {
            if (t.joinable()) {
                allFinished = false;
                break;
            }
        }
        if (allFinished) break;

        // Show progress
        Console::ClearLine();
        unsigned long long attempts = context.GetAttempts();
        unsigned long long elapsed = timer.GetElapsed();
        std::cout << "Attempts: " << attempts << " | ";
        if (elapsed > 0) {
            std::cout << std::fixed << std::setprecision(1)
                     << (attempts * 1000.0 / elapsed) << " pwd/sec";
        }
        std::cout << " | Threads: " << threadCount;
        std::cout.flush();
    }

    // Wait for all threads to finish
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

    timer.Stop();

    // Display results
    Console::ClearLine();
    if (context.IsFound()) {
        Console::PrintSuccess("PASSWORD FOUND!");
        std::cout << "\n";
        Console::PrintSeparator('=', 70);
        std::cout << "Login:        " << targetLogin << "\n";
        std::cout << "Password:     " << context.foundPassword << "\n";
        std::cout << "Threads:      " << threadCount << "\n";
        std::cout << "Attempts:     " << context.GetAttempts() << "\n";
        std::cout << "Time:         " << timer.GetElapsedFormatted() << "\n";
        std::cout << "Rate:         " << std::fixed << std::setprecision(1);
        unsigned long long elapsed = timer.GetElapsed();
        if (elapsed > 0) {
            std::cout << (context.GetAttempts() * 1000.0 / elapsed) << " passwords/second\n";
        }
        Console::PrintSeparator('=', 70);
    } else {
        Console::PrintWarning("Password not found!");
        std::cout << "Completed " << context.GetAttempts() << " attempts in "
                  << timer.GetElapsedFormatted() << "\n";
    }

    Console::PrintInfo("All threads finished");
}

// ============= Mode 3: Dictionary Attack with Rules =============

void ModeRuleBasedAttack() {
    Console::PrintHeader("DICTIONARY ATTACK WITH RULES");

    RuleBasedAttack attack;

    // Select loading mode
    int loadChoice = 0;
    std::cout << "\nSelect loading mode:\n";
    std::cout << " 1. Config file (rule -> vocabulary mapping)\n";
    std::cout << " 2. Single dictionary (all rules applied)\n";
    std::cout << "Enter choice (1-2): ";
    std::cin >> loadChoice;
    std::cin.ignore(10000, '\n');

    bool loaded = false;
    if (loadChoice == 1) {
        Console::PrintInfo("Select configuration file...");
        if (attack.LoadConfigFile()) {
            Console::PrintSuccess("Config loaded: " + std::to_string(attack.GetRuleSetCount()) + " rule sets");
            loaded = true;
        } else {
            Console::PrintError("No file selected or invalid config!");
            return;
        }
    } else {
        Console::PrintInfo("Select dictionary file...");
        if (attack.LoadDictionaryFromFile()) {
            Console::PrintSuccess("Dictionary loaded: " + std::to_string(attack.GetDictionarySize()) + " entries");
            loaded = true;
        } else {
            Console::PrintError("No file selected or failed to open!");
            return;
        }
    }

    if (!loaded) return;

    Console::PrintInfo("Generating password variants with rules...");
    attack.GenerateVariants();
    Console::PrintSuccess("Generated " + std::to_string(attack.GetVariantCount()) + " password variants");

    // Get target login
    std::string targetLogin;
    std::cout << "Enter login to crack: ";
    std::getline(std::cin, targetLogin);

    // Connect to server
    Console::PrintInfo("Connecting to server...");
    PipeClient client;
    if (!client.Connect()) {
        Console::PrintError("Failed to connect to server!");
        return;
    }
    Console::PrintSuccess("Connected!");

    Console::PrintInfo("Starting dictionary attack...");

    Timer timer;
    timer.Start();

    std::string password;
    unsigned long long attemptCount = 0;
    unsigned long long progressInterval = 100;
    bool found = false;

    while ((password = attack.Next()) != "" && !found) {
        attemptCount++;

        if (client.TryPassword(targetLogin, password)) {
            timer.Stop();
            found = true;

            Console::ClearLine();
            Console::PrintSuccess("PASSWORD FOUND!");
            std::cout << "\n";
            Console::PrintSeparator('=', 70);
            std::cout << "Login:        " << targetLogin << "\n";
            std::cout << "Password:     " << password << "\n";
            std::cout << "Attempts:     " << attemptCount << "\n";
            std::cout << "Time:         " << timer.GetElapsedFormatted() << "\n";
            std::cout << "Rate:         " << std::fixed << std::setprecision(1);
            unsigned long long elapsedMs = timer.GetElapsed();
            if (elapsedMs > 0) {
                std::cout << (attemptCount * 1000.0 / elapsedMs) << " passwords/second\n";
            } else {
                std::cout << "-- passwords/second\n";
            }
            Console::PrintSeparator('=', 70);
            break;
        }

        // Show progress
        if (attemptCount % progressInterval == 0) {
            Console::ClearLine();
            unsigned long long elapsedTime = timer.GetElapsed();
            std::cout << "Attempt " << attemptCount << ": " << password
                      << " | " << std::fixed << std::setprecision(1);
            if (elapsedTime > 0) {
                std::cout << (attemptCount * 1000.0 / elapsedTime) << " pwd/sec";
            } else {
                std::cout << "-- pwd/sec";
            }
            std::cout.flush();
        }
    }

    if (!found) {
        timer.Stop();
        Console::ClearLine();
        Console::PrintWarning("Password not found in dictionary!");
        std::cout << "Tested " << attemptCount << " variants in "
                  << timer.GetElapsedFormatted() << "\n";
    }

    client.Disconnect();
    Console::PrintInfo("Disconnected from server");
}
