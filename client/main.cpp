#include <iostream>
#include <iomanip>
#include "PipeClient.h"
#include "BruteForce.h"
#include "RuleAttack.h"
#include "Utils.h"

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
        std::cout << "\nEnter your choice (1-3, 0 to exit): ";
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
    std::cout << " 1. Connection Test     - Test connectivity and single credential\n";
    std::cout << " 2. Brute Force Attack  - Try all possible character combinations\n";
    std::cout << " 3. Dictionary Attack   - Test passwords from file with rules\n";
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
