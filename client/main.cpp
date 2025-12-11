#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include "PipeClient.h"
#include "BruteForce.h"
#include "RuleAttack.h"
#include "Utils.h"

// Global shared state for multithreading
std::atomic<bool> g_passwordFound(false);
std::atomic<unsigned long long> g_totalAttempts(0);
std::mutex g_consoleMutex;
std::string g_foundPassword;

// Forward declarations
void ShowMainMenu();
void ModeConnectionTest();
void ModeBruteForceMultithreaded();
void ModeRuleBasedAttackMultithreaded();

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
                ModeBruteForceMultithreaded();
                break;
            case 3:
                ModeRuleBasedAttackMultithreaded();
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
    std::cout << " PASSWORD CRACKING CLIENT (140% Grade - Multithreaded)\n";
    Console::PrintSeparator('=', 70);
    std::cout << " 1. Connection Test       - Verify server connectivity\n";
    std::cout << " 2. Brute Force Attack    - Try all combinations (multithreaded)\n";
    std::cout << " 3. Dictionary Attack     - Rule-based attack (multithreaded)\n";
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

    if (maxLength < 1 || maxLength > 20) {
        Console::PrintError("Invalid length! Maximum is 20 characters.");
        return;
    }

    // Get login to crack
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

    // Create brute force generator
    BruteForceGenerator generator(alphabet, maxLength);

    Console::PrintInfo("Starting brute force attack...");
    std::cout << "Target: " << targetLogin << "\n";
    std::cout << "Alphabet size: " << alphabet.length() << " characters\n";
    std::cout << "Max length: " << maxLength << "\n";
    std::cout << "Total combinations: " << generator.GetTotalCombinations() << "\n\n";

    // Attack loop
    Timer timer;
    timer.Start();

    std::string password;
    unsigned long long progressInterval = 100; // Show progress every 100 attempts
    bool found = false;

    while (generator.HasNext() && !found) {
        password = generator.Next();

        // Try password
        if (client.TryPassword(targetLogin, password)) {
            timer.Stop();
            found = true;

            // Clear progress line and show result
            Console::ClearLine();
            Console::PrintSuccess("PASSWORD FOUND!");
            std::cout << "\n";
            Console::PrintSeparator('=', 70);
            std::cout << "Login:        " << targetLogin << "\n";
            std::cout << "Password:     " << password << "\n";
            std::cout << "Attempts:     " << generator.GetAttemptCount() << "\n";
            std::cout << "Time:         " << timer.GetElapsedFormatted() << "\n";
            std::cout << "Rate:         " << std::fixed << std::setprecision(1);
            unsigned long long elapsed = timer.GetElapsed();
            if (elapsed > 0) {
                std::cout << (generator.GetAttemptCount() * 1000.0 / elapsed) << " passwords/second\n";
            } else {
                std::cout << "-- passwords/second\n";
            }
            Console::PrintSeparator('=', 70);
            break;
        }

        // Show progress every N attempts
        if (generator.GetAttemptCount() % progressInterval == 0) {
            Console::ClearLine();
            unsigned long long elapsed = timer.GetElapsed();
            std::cout << "Attempt " << generator.GetAttemptCount()
                      << ": " << password
                      << " | " << std::fixed << std::setprecision(1);
            if (elapsed > 0) {
                std::cout << (generator.GetAttemptCount() * 1000.0 / elapsed) << " pwd/sec";
            } else {
                std::cout << "-- pwd/sec";
            }
            std::cout.flush();
        }
    }

    if (!found) {
        timer.Stop();
        Console::ClearLine();
        Console::PrintWarning("Password not found!");
        std::cout << "Completed " << generator.GetAttemptCount() << " attempts in "
                  << timer.GetElapsedFormatted() << "\n";
    }

    client.Disconnect();
    Console::PrintInfo("Disconnected from server");
}

// ============= Mode 3: Dictionary Attack with Rules =============

void ModeRuleBasedAttack() {
    Console::PrintHeader("DICTIONARY ATTACK WITH RULES");

    RuleBasedAttack attack;

    Console::PrintInfo("Select dictionary file...");
    if (!attack.LoadDictionaryFromFile()) {
        Console::PrintError("No file selected or failed to open!");
        return;
    }

    Console::PrintSuccess("Dictionary loaded: " + std::to_string(attack.GetDictionarySize()) + " entries");

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

// ============= Worker Thread for Brute Force =============

void BruteForceWorkerThread(
    int threadId,
    unsigned long long startIndex,
    unsigned long long endIndex,
    const std::string& login,
    const std::string& alphabet,
    int maxLength
) {
    PipeClient client;
    if (!client.Connect()) {
        std::lock_guard<std::mutex> lock(g_consoleMutex);
        Console::PrintError("Thread " + std::to_string(threadId) + " failed to connect");
        return;
    }

    BruteForceGenerator generator(alphabet, maxLength);

    for (unsigned long long i = startIndex; i < endIndex; i++) {
        if (g_passwordFound.load()) {
            client.Disconnect();
            return;
        }

        std::string password = generator.GetPasswordAtIndex(i);
        if (password.empty()) continue;

        if (client.TryPassword(login, password)) {
            g_passwordFound.store(true);
            g_foundPassword = password;
            
            std::lock_guard<std::mutex> lock(g_consoleMutex);
            Console::ClearLine();
            Console::PrintSuccess("Thread " + std::to_string(threadId) + " found password: " + password);
            client.Disconnect();
            return;
        }

        g_totalAttempts.fetch_add(1);

        if (i % 100 == 0) {
            std::lock_guard<std::mutex> lock(g_consoleMutex);
            Console::ClearLine();
            std::cout << "Thread " << threadId << " progress: " 
                      << (i - startIndex) << "/" << (endIndex - startIndex)
                      << " | Total attempts: " << g_totalAttempts.load();
            std::cout.flush();
        }
    }

    client.Disconnect();
}

// ============= Mode 4: Multithreaded Brute Force =============

void ModeBruteForceMultithreaded() {
    Console::PrintHeader("MULTITHREADED BRUTE FORCE ATTACK");

    // Select alphabet
    int alphabetChoice = 0;
    std::cout << "\nSelect alphabet:\n";
    std::cout << " 1. Lowercase + '      (27 characters)\n";
    std::cout << " 2. Letters + Digits + ' (63 characters)\n";
    std::cout << " 3. Full Alphabet      (128 characters)\n";
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
            alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
                      "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюя'";
            break;
        default:
            Console::PrintError("Invalid choice!");
            return;
    }

    int maxLength = 0;
    std::cout << "Enter maximum password length (1-20): ";
    std::cin >> maxLength;
    std::cin.ignore(10000, '\n');

    if (maxLength < 1 || maxLength > 20) {
        Console::PrintError("Invalid length!");
        return;
    }

    std::string targetLogin;
    std::cout << "Enter login to crack: ";
    std::getline(std::cin, targetLogin);

    // Determine number of threads
    int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::cout << "\nUse how many threads? (Available: " << numThreads << "): ";
    int userThreads = 0;
    std::cin >> userThreads;
    std::cin.ignore(10000, '\n');

    if (userThreads > 0 && userThreads <= 64) {
        numThreads = userThreads;
    }

    BruteForceGenerator tempGen(alphabet, maxLength);
    unsigned long long totalCombos = tempGen.GetTotalCombinations();

    Console::PrintInfo("Starting multithreaded attack...");
    std::cout << "Threads:      " << numThreads << "\n";
    std::cout << "Target:       " << targetLogin << "\n";
    std::cout << "Alphabet:     " << alphabet.length() << " characters\n";
    std::cout << "Max length:   " << maxLength << "\n";
    std::cout << "Total combos: " << totalCombos << "\n\n";

    // Reset global state
    g_passwordFound.store(false);
    g_totalAttempts.store(0);
    g_foundPassword.clear();

    Timer timer;
    timer.Start();

    std::vector<std::thread> threads;
    unsigned long long chunkSize = totalCombos / numThreads;

    for (int i = 0; i < numThreads; i++) {
        unsigned long long start = i * chunkSize + 1;
        unsigned long long end = (i == numThreads - 1) ? totalCombos + 1 : start + chunkSize;

        threads.emplace_back(BruteForceWorkerThread, i, start, end, 
                           targetLogin, alphabet, maxLength);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    timer.Stop();

    Console::ClearLine();
    if (g_passwordFound.load()) {
        Console::PrintSuccess("PASSWORD FOUND!");
        std::cout << "\n";
        Console::PrintSeparator('=', 70);
        std::cout << "Login:        " << targetLogin << "\n";
        std::cout << "Password:     " << g_foundPassword << "\n";
        std::cout << "Threads:      " << numThreads << "\n";
        std::cout << "Total attempts: " << g_totalAttempts.load() << "\n";
        std::cout << "Time:         " << timer.GetElapsedFormatted() << "\n";
        std::cout << "Rate:         " << std::fixed << std::setprecision(1);
        unsigned long long elapsed = timer.GetElapsed();
        if (elapsed > 0) {
            std::cout << (g_totalAttempts.load() * 1000.0 / elapsed) << " passwords/second\n";
        }
        Console::PrintSeparator('=', 70);
    } else {
        Console::PrintWarning("Password not found!");
        std::cout << "Tested " << g_totalAttempts.load() << " passwords in "
                  << timer.GetElapsedFormatted() << "\n";
    }
}

// ============= Mode 5: Multithreaded Dictionary Attack =============

void ModeRuleBasedAttackMultithreaded() {
    Console::PrintHeader("MULTITHREADED DICTIONARY ATTACK");

    RuleBasedAttack attack;

    Console::PrintInfo("Select dictionary file...");
    if (!attack.LoadDictionaryFromFile()) {
        Console::PrintError("No file selected or failed to open!");
        return;
    }

    Console::PrintSuccess("Dictionary loaded: " + std::to_string(attack.GetDictionarySize()) + " entries");

    Console::PrintInfo("Generating password variants with rules...");
    attack.GenerateVariants();
    Console::PrintSuccess("Generated " + std::to_string(attack.GetVariantCount()) + " password variants");

    std::string targetLogin;
    std::cout << "Enter login to crack: ";
    std::getline(std::cin, targetLogin);

    int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::cout << "\nUse how many threads? (Available: " << numThreads << "): ";
    int userThreads = 0;
    std::cin >> userThreads;
    std::cin.ignore(10000, '\n');

    if (userThreads > 0 && userThreads <= 64) {
        numThreads = userThreads;
    }

    size_t totalVariants = attack.GetVariantCount();

    Console::PrintInfo("Starting multithreaded dictionary attack...");
    std::cout << "Threads:  " << numThreads << "\n";
    std::cout << "Target:   " << targetLogin << "\n";
    std::cout << "Variants: " << totalVariants << "\n\n";

    // Reset global state
    g_passwordFound.store(false);
    g_totalAttempts.store(0);
    g_foundPassword.clear();

    Timer timer;
    timer.Start();

    std::vector<std::thread> threads;
    size_t chunkSize = totalVariants / numThreads;

    // Simple approach: each thread tests a range of the variants
    for (int i = 0; i < numThreads; i++) {
        size_t start = i * chunkSize;
        size_t end = (i == numThreads - 1) ? totalVariants : start + chunkSize;

        threads.emplace_back([i, start, end, &targetLogin, &attack]() {
            PipeClient client;
            if (!client.Connect()) {
                std::lock_guard<std::mutex> lock(g_consoleMutex);
                Console::PrintError("Thread " + std::to_string(i) + " failed to connect");
                return;
            }

            for (size_t idx = start; idx < end; idx++) {
                if (g_passwordFound.load()) {
                    client.Disconnect();
                    return;
                }

                // Access variant by index - LinkedList supports operator[]
                std::string password = attack.GetVariantAtIndex(idx);

                if (client.TryPassword(targetLogin, password)) {
                    g_passwordFound.store(true);
                    g_foundPassword = password;
                    
                    std::lock_guard<std::mutex> lock(g_consoleMutex);
                    Console::ClearLine();
                    Console::PrintSuccess("Thread " + std::to_string(i) + " found password: " + password);
                    client.Disconnect();
                    return;
                }

                g_totalAttempts.fetch_add(1);

                if (idx % 100 == 0) {
                    std::lock_guard<std::mutex> lock(g_consoleMutex);
                    Console::ClearLine();
                    std::cout << "Thread " << i << " progress: " 
                              << (idx - start) << "/" << (end - start)
                              << " | Total: " << g_totalAttempts.load();
                    std::cout.flush();
                }
            }

            client.Disconnect();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    timer.Stop();

    Console::ClearLine();
    if (g_passwordFound.load()) {
        Console::PrintSuccess("PASSWORD FOUND!");
        std::cout << "\n";
        Console::PrintSeparator('=', 70);
        std::cout << "Login:        " << targetLogin << "\n";
        std::cout << "Password:     " << g_foundPassword << "\n";
        std::cout << "Threads:      " << numThreads << "\n";
        std::cout << "Total attempts: " << g_totalAttempts.load() << "\n";
        std::cout << "Time:         " << timer.GetElapsedFormatted() << "\n";
        std::cout << "Rate:         " << std::fixed << std::setprecision(1);
        unsigned long long elapsed = timer.GetElapsed();
        if (elapsed > 0) {
            std::cout << (g_totalAttempts.load() * 1000.0 / elapsed) << " passwords/second\n";
        }
        Console::PrintSeparator('=', 70);
    } else {
        Console::PrintWarning("Password not found!");
        std::cout << "Tested " << g_totalAttempts.load() << " passwords in "
                  << timer.GetElapsedFormatted() << "\n";
    }
}
