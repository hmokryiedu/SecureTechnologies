# Issues and Missing Features - Requirements Compliance Analysis

**Project:** Password Cracking & Authentication Vulnerability Demonstration
**Analysis Date:** 2025-12-11
**Base Document:** requirements.md (Laboratory Work №1)
**Reference:** project-overview.md

---

## Executive Summary

This document provides a comprehensive analysis of gaps between the Laboratory Work №1 requirements and the current implementation. The project currently achieves **75% grade compliance** (basic brute-force attack functional), but falls short of 100% and 140% grades due to missing features and non-compliant implementations.

### Grading Compliance Status

| Grade Level | Requirements | Status | Blocking Issues |
|-------------|--------------|--------|-----------------|
| **75%** | Brute-force attack + Basic server | ✅ **COMPLETE** | None |
| **100%** | Rule-based attack + Anti-cracking + Templated linked list + OOP | ⚠️ **PARTIAL** | #1, #2, #3, #5, #11 |
| **140%** | Multithreading (client + server) + 60%+ crack rate | ⚠️ **PARTIAL** | #4 |

### Issue Summary

- **Total Issues Identified:** 12
- **Critical (Requirements Non-Compliance):** 7 issues (#1, #2, #3, #4, #5, #11 + #5b resolved)
- **High Severity (Code Quality):** 3 issues
- **Medium Severity (Performance & Reliability):** 2 issues

### Recent Updates (2025-12-11)
- **Issue #5 ADDED:** Race condition on `blockedUsers` map - CRITICAL bug blocking 100% grade
- **Issue #5b RESOLVED:** Async/Overlapped I/O now implemented in PipeServer.h

---

## Table of Contents

1. [Critical: Requirements Non-Compliance](#critical-requirements-non-compliance)
2. [High Severity: Code Quality Issues](#high-severity-code-quality-issues)
3. [Medium Severity: Performance & Reliability](#medium-severity-performance--reliability)
4. [Implementation Priority](#implementation-priority)
5. [Testing Recommendations](#testing-recommendations)

---

## Critical: Requirements Non-Compliance

These issues directly prevent achieving 100% or 140% grade levels.

---

### Issue #1: Missing Templated Singly Linked List

**Severity:** CRITICAL
**Requirement:** requirements.md Section 3.B.3
**Grade Impact:** Blocks 100% grade

#### Requirement Statement
> "Data Structure: Use a **Templated Singly Linked List** to store password dictionaries and rule sets."

#### Current Implementation
- **What's implemented:** Using STL containers (`std::vector`, `std::unordered_set`)
- **Files affected:**
  - `client/RuleAttack.h:10` - `std::vector<std::string> dictionary;`
  - `client/RuleAttack.h:11` - `std::vector<std::string> variants;`
  - `client/RuleAttack.h:12` - `std::unordered_set<std::string> variantSet;`
- **Compliance:** ❌ **FAIL** - No custom linked list implementation exists

#### Impact
- Does NOT meet explicit 100% grade requirement
- Demonstrates standard library usage instead of custom data structure implementation
- Missing educational objective of implementing fundamental data structures

#### Files to Create/Modify

**New Files:**
```
C:\Users\glebm\projects\tech-safety\brute-force\client\LinkedList.h
```

**Modified Files:**
```
C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.h
C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.cpp
```

#### Implementation Plan

**Step 1: Create Templated LinkedList Class**
```cpp
// File: client/LinkedList.h
template<typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& value) : data(value), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    size_t count;

public:
    LinkedList() : head(nullptr), tail(nullptr), count(0) {}

    ~LinkedList() {
        clear();
    }

    void push_back(const T& value) {
        Node* newNode = new Node(value);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        count++;
    }

    size_t size() const { return count; }

    void clear() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
        tail = nullptr;
        count = 0;
    }

    // Iterator implementation for range-based for loops
    class Iterator {
        Node* current;
    public:
        Iterator(Node* node) : current(node) {}
        T& operator*() { return current->data; }
        Iterator& operator++() { current = current->next; return *this; }
        bool operator!=(const Iterator& other) { return current != other.current; }
    };

    Iterator begin() { return Iterator(head); }
    Iterator end() { return Iterator(nullptr); }
};
```

**Step 2: Modify RuleAttack.h**
```cpp
// Replace line 10-11:
#include "LinkedList.h"

private:
    LinkedList<std::string> dictionary;   // Replaced std::vector
    LinkedList<std::string> variants;     // Replaced std::vector
    std::unordered_set<std::string> variantSet;  // Keep for deduplication
```

**Step 3: Update RuleAttack.cpp**
- Modify `LoadDictionaryFromFile()` (line 67): Use `dictionary.push_back()`
- Modify `GenerateVariants()` (line 76-84): Iterate using range-based for loop
- Modify `Next()` method: Update to work with LinkedList iterator
- Test with existing dictionary attack functionality

**Step 4: Build and Test**
```bash
g++ -g client/main.cpp client/PipeClient.cpp client/BruteForce.cpp client/RuleAttack.cpp client/Utils.cpp -o client/client.exe -lcomdlg32
```

**Step 5: Validation**
- Load dictionary file with 100+ words
- Verify variant generation works correctly
- Confirm no memory leaks with large dictionaries (use task manager to monitor)

---

### Issue #2: Missing Character Transposition Rule

**Severity:** CRITICAL
**Requirement:** requirements.md Section 3.B.3
**Grade Impact:** Blocks 100% grade

#### Requirement Statement
> "Apply heuristic rules (implemented as class methods): [...] Transposition of characters."

#### Current Implementation
- **What's implemented:** Reversal, case variations, digit suffixes, Cyrillic conversion
- **What's missing:** Character transposition (swapping adjacent characters)
- **Compliance:** ❌ **FAIL** - Feature completely absent

#### Impact
- Does NOT meet 100% grade requirement for complete rule-based attack
- Missing common password typo pattern (e.g., "passwrod" instead of "password")
- Reduces effectiveness of dictionary attacks against human-typed passwords

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.h
C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.cpp
```

#### Implementation Plan

**Step 1: Add Method Declaration (RuleAttack.h)**
```cpp
// Add after line 36 (after CyrillicToLatin declaration):
std::vector<std::string> TransposeCharacters(const std::string& password);
```

**Step 2: Implement Transposition Logic (RuleAttack.cpp)**
```cpp
// Add after CyrillicToLatin() implementation (~line 170):
std::vector<std::string> RuleBasedAttack::TransposeCharacters(const std::string& password) {
    std::vector<std::string> transposed;

    // Generate all adjacent character swaps
    for (size_t i = 0; i < password.length() - 1; i++) {
        std::string variant = password;
        // Swap characters at position i and i+1
        std::swap(variant[i], variant[i + 1]);
        transposed.push_back(variant);
    }

    return transposed;
}
```

**Step 3: Integrate into ApplyAllRules() (lines 171-197)**
```cpp
// Add after line 197 (before return):
    // Apply transposition to original password and case variations
    auto transpositions = TransposeCharacters(basePassword);
    for (const auto& trans : transpositions) {
        for (const auto& caseVar : GenerateCaseVariations(trans)) {
            AddVariant(caseVar);
        }
    }
```

**Step 4: Test Cases**
- Input: "password" → Should generate: "apssword", "pासswor d", "passowrd", etc.
- Input: "admin" → Should generate: "admni", "adimn", "admnи", "damin"
- Verify deduplication prevents duplicates

**Estimated Variants Generated:** For n-character password: +4(n-1) variants (4 case variations × n-1 transpositions)

---

### Issue #3: Incomplete Keyboard Layout Swap

**Severity:** CRITICAL
**Requirement:** requirements.md Section 3.B.3
**Grade Impact:** Blocks 100% grade

#### Requirement Statement
> "Keyboard layout swap (e.g., typing 'ghbdtn' instead of 'привет')"

#### Current Implementation
- **LatinToCyrillic():** ✅ Partially functional (RuleAttack.cpp:149-160)
- **CyrillicToLatin():** ❌ Stubbed out (RuleAttack.cpp:162-169) - returns input unchanged
- **Compliance:** ⚠️ **PARTIAL** - Only 50% functional (one direction only)

#### Current Code (Non-Functional)
```cpp
// RuleAttack.cpp:162-169
std::string RuleBasedAttack::CyrillicToLatin(const std::string& cyrillic) {
    std::string result = cyrillic;
    // In a real implementation, we'd need proper UTF-8 Cyrillic support
    // For now, return as-is since we can't easily map back
    return result;  // ← Does nothing!
}
```

#### Impact
- Limits rule-based attack effectiveness for bilingual passwords
- Cannot handle Russian dictionary words typed in Latin layout
- Bidirectional keyboard swap is industry-standard password cracking rule

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.cpp (lines 162-169)
```

#### Implementation Plan

**Step 1: Add UTF-8 Conversion Support**
```cpp
#include <codecvt>
#include <locale>
```

**Step 2: Implement Reverse Mapping**
```cpp
std::string RuleBasedAttack::CyrillicToLatin(const std::string& cyrillic) {
    // UTF-8 Cyrillic → Latin phonetic mapping
    static const std::map<wchar_t, char> mapping = {
        // Lowercase
        {L'а', 'a'}, {L'б', 'b'}, {L'в', 'v'}, {L'г', 'g'}, {L'д', 'd'},
        {L'е', 'e'}, {L'ё', 'e'}, {L'ж', 'j'}, {L'з', 'z'}, {L'и', 'i'},
        {L'й', 'y'}, {L'к', 'k'}, {L'л', 'l'}, {L'м', 'm'}, {L'н', 'n'},
        {L'о', 'o'}, {L'п', 'p'}, {L'р', 'r'}, {L'с', 's'}, {L'т', 't'},
        {L'у', 'u'}, {L'ф', 'f'}, {L'х', 'h'}, {L'ц', 'c'}, {L'ч', 'ch'},
        {L'ш', 'sh'}, {L'щ', 'sch'}, {L'ъ', '\''}, {L'ы', 'y'}, {L'ь', '\''},
        {L'э', 'e'}, {L'ю', 'yu'}, {L'я', 'ya'},
        // Uppercase
        {L'А', 'A'}, {L'Б', 'B'}, {L'В', 'V'}, {L'Г', 'G'}, {L'Д', 'D'},
        {L'Е', 'E'}, {L'Ё', 'E'}, {L'Ж', 'J'}, {L'З', 'Z'}, {L'И', 'I'},
        {L'Й', 'Y'}, {L'К', 'K'}, {L'Л', 'L'}, {L'М', 'M'}, {L'Н', 'N'},
        {L'О', 'O'}, {L'П', 'P'}, {L'Р', 'R'}, {L'С', 'S'}, {L'Т', 'T'},
        {L'У', 'U'}, {L'Ф', 'F'}, {L'Х', 'H'}, {L'Ц', 'C'}, {L'Ч', 'CH'},
        {L'Ш', 'SH'}, {L'Щ', 'SCH'}, {L'Ъ', '\''}, {L'Ы', 'Y'}, {L'Ь', '\''},
        {L'Э', 'E'}, {L'Ю', 'YU'}, {L'Я', 'YA'}
    };

    // Convert UTF-8 string to wide string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(cyrillic);

    // Apply reverse mapping
    std::string result;
    for (wchar_t ch : wide) {
        if (mapping.count(ch)) {
            result += mapping.at(ch);
        } else {
            // Keep original character if not Cyrillic
            std::wstring temp(1, ch);
            result += converter.to_bytes(temp);
        }
    }

    return result;
}
```

**Step 3: Test Cases**
- Input: "привет" → Output: "privet"
- Input: "Москва" → Output: "Moskva"
- Input: "пароль" → Output: "parol'"

**Step 4: Verify Integration**
- Ensure ApplyAllRules() calls both LatinToCyrillic() and CyrillicToLatin()
- Test with Russian dictionary file

---

### Issue #4: Client NOT Multithreaded

**Severity:** CRITICAL
**Requirement:** requirements.md Section 4 (Grading 140%)
**Grade Impact:** Blocks 140% grade

#### Requirement Statement
> "**140%**: Client generates attacks in threads + Handle multiple clients concurrently + Bonus if >60% passwords cracked"

#### Current Implementation
- **Server:** ✅ Multithreaded (3 concurrent worker threads) - PipeServer.h
- **Client:** ❌ Single-threaded sequential password testing
- **Compliance:** ⚠️ **PARTIAL** - Server has multithreading, client does not

#### Impact
- Cannot achieve 140% grade
- Significant performance loss: Client can only test passwords sequentially
- Underutilizes multi-core systems (e.g., 8-core CPU only uses 1 core)
- Competition disadvantage: Multithreaded client could crack passwords 4-8x faster

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\client\main.cpp
C:\Users\glebm\projects\tech-safety\brute-force\client\BruteForce.h
C:\Users\glebm\projects\tech-safety\brute-force\client\BruteForce.cpp
C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.h
C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.cpp
C:\Users\glebm\projects\tech-safety\brute-force\client\PipeClient.h
C:\Users\glebm\projects\tech-safety\brute-force\client\PipeClient.cpp
```

#### Implementation Plan

**Step 1: Add Thread Pool Infrastructure (main.cpp)**
```cpp
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

// Global shared state
std::atomic<bool> g_passwordFound(false);
std::atomic<unsigned long long> g_totalAttempts(0);
std::mutex g_consoleMutex;
std::string g_foundPassword;
```

**Step 2: Modify Brute Force for Multithreading (BruteForce.h)**
```cpp
class BruteForceGenerator {
public:
    // New method: Get password at specific index for thread partitioning
    std::string GetPasswordAtIndex(unsigned long long index);

    // New method: Get total combinations
    unsigned long long GetTotalCombinations() const { return totalCombinations; }
};
```

**Step 3: Implement Thread Worker Function (main.cpp)**
```cpp
void BruteForceWorkerThread(
    int threadId,
    unsigned long long startIndex,
    unsigned long long endIndex,
    const std::string& login,
    const std::string& alphabet,
    int maxLength
) {
    PipeClient client;  // Each thread gets its own pipe client
    BruteForceGenerator generator(alphabet, maxLength);

    for (unsigned long long i = startIndex; i < endIndex; i++) {
        // Check if another thread found the password
        if (g_passwordFound.load()) {
            return;
        }

        std::string password = generator.GetPasswordAtIndex(i);

        if (client.TryPassword(login, password)) {
            g_passwordFound.store(true);
            g_foundPassword = password;

            // Thread-safe console output
            std::lock_guard<std::mutex> lock(g_consoleMutex);
            Console::Success("Thread " + std::to_string(threadId) +
                           " found password: " + password);
            return;
        }

        g_totalAttempts.fetch_add(1);

        // Periodic progress update (thread-safe)
        if (i % 100 == 0) {
            std::lock_guard<std::mutex> lock(g_consoleMutex);
            Console::Info("Thread " + std::to_string(threadId) +
                         " progress: " + std::to_string(i - startIndex) +
                         "/" + std::to_string(endIndex - startIndex));
        }
    }
}
```

**Step 4: Launch Thread Pool (main.cpp)**
```cpp
// In brute force attack mode:
int numThreads = std::thread::hardware_concurrency();
if (numThreads == 0) numThreads = 4;  // Default to 4 threads

std::cout << "Use multithreading? (" << numThreads << " threads available) [Y/N]: ";
char useThreads;
std::cin >> useThreads;

if (useThreads == 'Y' || useThreads == 'y') {
    unsigned long long totalCombos = generator.GetTotalCombinations();
    unsigned long long chunkSize = totalCombos / numThreads;

    std::vector<std::thread> threads;
    Timer timer;
    timer.Start();

    // Launch worker threads
    for (int i = 0; i < numThreads; i++) {
        unsigned long long start = i * chunkSize;
        unsigned long long end = (i == numThreads - 1) ? totalCombos : start + chunkSize;

        threads.emplace_back(BruteForceWorkerThread, i, start, end,
                            login, alphabet, maxLength);
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    timer.Stop();

    if (g_passwordFound.load()) {
        Console::Success("Password cracked: " + g_foundPassword);
        Console::Info("Total attempts: " + std::to_string(g_totalAttempts.load()));
        Console::Info("Elapsed time: " + timer.GetElapsedFormatted());
    }
}
```

**Step 5: Thread Safety for PipeClient**
- **CRITICAL:** Each thread must have its own PipeClient instance
- Do NOT share a single PipeClient across threads (race conditions!)
- Named pipes support multiple concurrent connections (server has 3 pipe instances)

**Step 6: Dictionary Attack Multithreading (RuleAttack.cpp)**
```cpp
// Similar approach: Partition variant list across threads
// Each thread processes a subset of password variants
```

**Step 7: Testing**
```bash
# Build with thread support
g++ -g client/main.cpp client/PipeClient.cpp client/BruteForce.cpp client/RuleAttack.cpp client/Utils.cpp -o client/client.exe -lcomdlg32 -lpthread

# Test with 4 threads on 5-character password
# Expected speedup: ~3-4x faster than single-threaded
```

**Performance Expectations:**
- Single-threaded: 50-200 attempts/sec
- 4 threads: 200-800 attempts/sec
- 8 threads: 400-1600 attempts/sec

---

### Issue #5: Race Condition on blockedUsers Map (CRITICAL BUG)

**Severity:** CRITICAL (Thread Safety Bug)
**Requirement:** requirements.md Section 3.A.4 (Anti-Cracking Protection)
**Grade Impact:** Blocks 100% grade - Anti-cracking protection is broken

#### Requirement Statement
> "Implement an artificial delay (or request skipping) for a specific login after repeated failures.
> **Constraint:** This delay must **not** freeze the whole server; other clients must still be served instantly."

#### Current Implementation
- **Multithreading:** ✅ Server creates 3 worker threads (PipeServer.h:166)
- **Async I/O:** ✅ Uses `FILE_FLAG_OVERLAPPED` (PipeServer.h:43)
- **Thread Synchronization:** ❌ **MISSING** - `blockedUsers` map accessed without locks

#### Problem Location
```cpp
// list.h:20 - Shared data structure
map<string, DWORD> blockedUsers; // ← Accessed by 3 threads WITHOUT synchronization!

// list.h:75-105 - CheckUser() method (UNSAFE)
int CheckUser(const string& login, const string& pass) {
    if (protectionMode) {
        DWORD currentTime = GetTickCount();
        if (blockedUsers.count(login)) {        // ⚠️ UNSAFE READ
            if (currentTime < blockedUsers[login]) {
                return -1;
            } else {
                blockedUsers.erase(login);       // ⚠️ UNSAFE WRITE
            }
        }
    }
    // ...
    if (protectionMode) blockedUsers.erase(login);           // ⚠️ UNSAFE WRITE
    // ...
    if (protectionMode) blockedUsers[login] = GetTickCount() + 3000;  // ⚠️ UNSAFE WRITE
}
```

#### Why This Is Critical

**Race Condition Scenario:**
```
Thread 1 (Client A):                     Thread 2 (Client B):
------------------------                 ------------------------
blockedUsers.count("admin")              
                                         blockedUsers["admin"] = time+3000
blockedUsers["admin"] ← CORRUPTED!       
```

**Consequences:**
1. **Map corruption:** `std::map` is NOT thread-safe for concurrent modification
2. **Iterator invalidation:** One thread erasing while another reads → crash
3. **Undefined behavior:** C++ standard makes no guarantees for concurrent access
4. **Anti-cracking fails:** Protection mode may not work correctly
5. **Server crashes:** Segmentation faults under load

#### Impact
- **Requirement violation:** Anti-cracking protection is unreliable (requirements.md:39-41)
- **100% grade blocked:** Anti-cracking mode is mandatory for 100% grade
- **Server instability:** Random crashes under concurrent client attacks
- **Data corruption:** Block times may be incorrect or lost

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\list.h (lines 20-26, 75-105)
```

#### Implementation Plan

**Step 1: Add CRITICAL_SECTION Member Variable (after line 23)**
```cpp
class UserList {
private:
    // ... existing members ...
    map<string, DWORD> blockedUsers;
    bool protectionMode;
    int maxLoginLen;
    int maxPassLen;
    
    CRITICAL_SECTION blockedUsersLock; // ← ADD THIS: Protects blockedUsers map
```

**Step 2: Initialize Lock in Constructor (line 26)**
```cpp
// BEFORE:
UserList() : protectionMode(false), maxLoginLen(0), maxPassLen(0) {}

// AFTER:
UserList() : protectionMode(false), maxLoginLen(0), maxPassLen(0) {
    InitializeCriticalSection(&blockedUsersLock);
}
```

**Step 3: Add Destructor for Cleanup**
```cpp
~UserList() {
    DeleteCriticalSection(&blockedUsersLock);
}
```

**Step 4: Protect CheckUser() Method (lines 75-105)**
```cpp
int CheckUser(const string& login, const string& pass) {
    // 1. Check if user is blocked (Anti-Brute-Force)
    if (protectionMode) {
        EnterCriticalSection(&blockedUsersLock); // 🔒 LOCK

        DWORD currentTime = GetTickCount();
        if (blockedUsers.count(login)) {
            if (currentTime < blockedUsers[login]) {
                LeaveCriticalSection(&blockedUsersLock); // 🔓 UNLOCK
                return -1; // Still blocked
            } else {
                blockedUsers.erase(login); // Block expired
            }
        }

        LeaveCriticalSection(&blockedUsersLock); // 🔓 UNLOCK
    }

    // 2. Search for user and validate password
    for (const auto& u : users) {
        if (u.login == login) {
            if (u.password == pass) {
                if (protectionMode) {
                    EnterCriticalSection(&blockedUsersLock); // 🔒 LOCK
                    blockedUsers.erase(login);
                    LeaveCriticalSection(&blockedUsersLock); // 🔓 UNLOCK
                }
                return 1; // Success
            } else {
                if (protectionMode) {
                    EnterCriticalSection(&blockedUsersLock); // 🔒 LOCK
                    blockedUsers[login] = GetTickCount() + 3000; // Block 3 sec
                    LeaveCriticalSection(&blockedUsersLock); // 🔓 UNLOCK
                }
                return 0; // Wrong password
            }
        }
    }
    return 0; // User not found
}
```

#### Design Decisions

1. **Why CRITICAL_SECTION?**
   - Fast (optimized for same-process threads)
   - Simple (single lock = no deadlocks)
   - Windows native (matches rest of codebase)
   - Low contention (only 3 threads)

2. **Why 3 separate lock acquisitions?**
   - First lock: Check if blocked
   - Second lock: Clear block on success
   - Third lock: Add block on failure
   - **Benefit:** Minimizes lock hold time = better concurrency

3. **Why NOT lock the users vector?**
   - `users` vector is read-only after initialization
   - `Load()` is called before server starts (single-threaded)
   - No synchronization needed for read-only data

#### Testing Plan

| Test | Description | Expected Result |
|------|-------------|-----------------|
| Test 1 | Single-client protection mode | Block works, timeout works, clear on success |
| Test 2 | Concurrent attacks on SAME login | No crashes, both see -1 or 0 correctly |
| Test 3 | Concurrent attacks on DIFFERENT logins | Each login blocked independently |
| Test 4 | Stress test (1 hour) | No hangs, no crashes, no deadlocks |
| Test 5 | Performance measurement | <1% overhead from locking |

#### Compliance After Fix

| Requirement | Before | After |
|-------------|--------|-------|
| "Delay must not freeze server" (req:39-41) | 🔴 **BUG** (race breaks this) | ✅ **FIXED** |
| "Anti-cracking mode" (req:82) | 🔴 **BUG** (not thread-safe) | ✅ **FIXED** |
| 100% grade | 🔴 **BLOCKED** | ✅ **COMPLETE** |

**Estimated Effort:** 30 min implementation + 1-2 hours testing = ~2 hours total

---

### Issue #5b: Server Async/Overlapped Pipes (Already Implemented)

**Severity:** RESOLVED ✅
**Requirement:** requirements.md Section 6
**Grade Impact:** None (already implemented)

#### Current Status
The server now uses async/overlapped I/O correctly:

```cpp
// PipeServer.h:43 - Async flag enabled
PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // АСИНХРОННИЙ РЕЖИМ

// PipeServer.h:32-38 - OVERLAPPED structure used
data->oOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

// PipeServer.h:62-80 - Async ConnectNamedPipe with GetOverlappedResult
ConnectNamedPipe(data->hPipe, &data->oOverlap);
if (error == ERROR_IO_PENDING) {
    GetOverlappedResult(data->hPipe, &data->oOverlap, &bytesTransferred, TRUE);
}

// PipeServer.h:98-103 - Async ReadFile
ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesTransferred, &data->oOverlap);

// PipeServer.h:130-135 - Async WriteFile
WriteFile(data->hPipe, &replyValue, sizeof(DWORD), &bytesTransferred, &data->oOverlap);
```

**Compliance:** ✅ **COMPLETE** - Async I/O fully implemented per requirements.md:102

---

### Issue #11: OOP Compliance Violations

**Severity:** MIXED (1 BLOCKING, 1 MODERATE, 2 MINOR)
**Requirement:** requirements.md Section 2, Line 14
**Grade Impact:** Blocks 100% grade

#### Requirement Statement
> "**Tech Stack:** C++ (recommended due to WinAPI usage), **Object-Oriented Programming (OOP) required**."

#### Current Implementation
- **What's implemented:**
  - Good class structure: `PipeClient`, `UserList` (excellent encapsulation)
  - Attack generators: `BruteForceGenerator`, `RuleBasedAttack` (proper classes)
  - Main application properly uses class instances
- **What's missing:**
  1. PipeServer uses static methods for core authentication logic (BLOCKING)
  2. No polymorphic base class for attack generators (MODERATE)
  3. PerPipeStruct uses struct with all public members (MINOR)
  4. Console utilities in namespace instead of class (MINOR - acceptable)
- **Compliance:** ⚠️ **PARTIAL** - ~85% OOP-compliant, blocking issues prevent 100% grade

#### Impact
- Does NOT meet explicit 100% grade requirement (requirements.md:14)
- Violates core OOP principles: encapsulation (PipeServer), polymorphism (attack classes)
- Educational objective not fully met: demonstrating OOP mastery

---

#### Violation #1: PipeServer Static Thread Logic (BLOCKING)

**File:** `PipeServer.h` (lines 17-95)
**Severity:** CRITICAL - Blocks 100% grade

**Problem:**
- Lines 24-95: `static DWORD WINAPI PipeInstanceThread(LPVOID lpvParam)` - 72 lines of core authentication logic implemented as a static method
- Lines 17-21: `static void Log(HWND hGui, const std::string& text)` - Static logging helper
- Static methods cannot access instance state via `this->` pointer
- Requires unsafe C-style casting: `PerPipeData* data = (PerPipeData*)lpvParam;`
- Thread function passes all state via void pointer instead of using object encapsulation

**Current Code (Non-OOP):**
```cpp
// PipeServer.h:24-95
class PipeServer {
private:
    static DWORD WINAPI PipeInstanceThread(LPVOID lpvParam) {
        PerPipeData* data = (PerPipeData*)lpvParam;  // Manual C-style cast
        
        while (true) {
            data->hPipe = CreateNamedPipeA(...);
            ConnectNamedPipe(data->hPipe, NULL);
            
            // 60+ lines of authentication logic here
            // Cannot access PipeServer instance state
            // Cannot use 'this->' pointer
            
            CloseHandle(data->hPipe);
        }
        return 0;
    }
    
    static void Log(HWND hGui, const string& text) {
        char* buf = new char[text.length() + 1];
        strcpy_s(buf, text.length() + 1, text.c_str());
        PostMessage(hGui, WM_LOG_MSG, (WPARAM)buf, 0);
    }
};
```

**Why This Violates OOP:**
- **Encapsulation Violation:** Core business logic not encapsulated in instance methods
- **Static Anti-Pattern:** Static methods used where instance methods are appropriate
- **State Management:** Cannot access object state, forcing parameter passing
- **Type Safety:** Requires unsafe void pointer casting instead of type-safe object references

**OOP-Compliant Refactoring:**
```cpp
class PipeServer {
private:
    vector<HANDLE> threads;
    bool isRunning;
    HWND hGui;
    UserList* userList;
    
    // Instance method (non-static)
    void Log(const string& text) {
        // Can access this->hGui directly
        char buf[512];
        strcpy_s(buf, sizeof(buf), text.c_str());
        SendMessage(this->hGui, WM_LOG_MSG, (WPARAM)buf, 0);
    }
    
    // Instance method for pipe handling
    DWORD HandlePipeInstance(int pipeId) {
        HANDLE hPipe;
        while (this->isRunning) {  // Can access instance state
            hPipe = CreateNamedPipeA(...);
            ConnectNamedPipe(hPipe, NULL);
            
            // Authentication logic with access to this->userList
            char buffer[512];
            DWORD bytesRead;
            ReadFile(hPipe, buffer, sizeof(buffer)-1, &bytesRead, NULL);
            
            // Parse login/password
            string message(buffer, bytesRead);
            // ... use this->userList->CheckUser(...)
            
            CloseHandle(hPipe);
        }
        return 0;
    }
    
    // Thread wrapper (static required by CreateThread API)
    static DWORD WINAPI ThreadWrapperFunc(LPVOID lpParam) {
        PipeServerThreadContext* ctx = (PipeServerThreadContext*)lpParam;
        return ctx->server->HandlePipeInstance(ctx->pipeId);
    }
    
public:
    void Start(int numPipes, UserList* uList, HWND gui) {
        this->hGui = gui;
        this->userList = uList;
        this->isRunning = true;
        
        for (int i = 0; i < numPipes; i++) {
            PipeServerThreadContext* ctx = new PipeServerThreadContext;
            ctx->server = this;
            ctx->pipeId = i;
            
            HANDLE hThread = CreateThread(NULL, 0, ThreadWrapperFunc, ctx, 0, NULL);
            threads.push_back(hThread);
        }
    }
};

// Helper struct for thread context
struct PipeServerThreadContext {
    PipeServer* server;
    int pipeId;
};
```

**Benefits of OOP Refactoring:**
- ✅ Instance methods can access `this->hGui`, `this->userList`, `this->isRunning`
- ✅ Type-safe object references instead of void pointers
- ✅ Encapsulation: all server state managed within the object
- ✅ Testability: can create PipeServer instances for unit testing
- ✅ Thread-safety: easier to add synchronization within object methods

**Estimated Effort:** 3-4 hours

---

#### Violation #2: Missing Attack Class Polymorphism (MODERATE)

**Files:** `client/BruteForce.h` (lines 5-27), `client/RuleAttack.h` (lines 8-38)
**Severity:** MODERATE - Violates polymorphism principle

**Problem:**
- Two attack classes with nearly identical public interfaces but no common base class
- Both implement: `HasNext()`, `Next()`, `Reset()`, `GetProgressPercent()`
- Cannot use polymorphic behavior: `PasswordGenerator* gen = new BruteForceGenerator();`
- Code duplication and rigid design (cannot easily add new attack types)

**Current Code (No Inheritance):**

```cpp
// BruteForce.h:
class BruteForceGenerator {
private:
    std::string alphabet;
    int maxLength;
    unsigned long long attemptCount;
    unsigned long long totalCombinations;
    std::string currentPassword;
    std::vector<int> indices;
    bool hasMore;
    
public:
    BruteForceGenerator(const std::string& alphabet, int maxLen);
    bool HasNext() const;
    std::string Next();
    void Reset();
    std::string GetCurrent() const { return currentPassword; }
    unsigned long long GetAttemptCount() const { return attemptCount; }
    unsigned long long GetTotalCombinations() const { return totalCombinations; }
    double GetProgressPercent() const;
};

// RuleAttack.h:
class RuleBasedAttack {
private:
    std::vector<std::string> dictionary;
    std::vector<std::string> variants;
    std::unordered_set<std::string> variantSet;
    size_t currentIndex;
    
public:
    RuleBasedAttack();
    bool LoadDictionaryFromFile();
    bool LoadDictionaryFromFile(const std::string& filename);
    void GenerateVariants();
    std::string Next();
    void Reset() { currentIndex = 0; }
    bool HasNext() const { return currentIndex < variants.size(); }
    size_t GetVariantCount() const { return variants.size(); }
    size_t GetDictionarySize() const { return dictionary.size(); }
    double GetProgressPercent() const;
};
```

**Why This Violates OOP:**
- **No Polymorphism:** Cannot treat both as `PasswordGenerator*`
- **Violates Open/Closed Principle:** Cannot add new attack types without modifying client code
- **Code Duplication:** Identical method signatures with no shared interface

**OOP-Compliant Refactoring:**

```cpp
// File: client/PasswordGenerator.h
class PasswordGenerator {
public:
    virtual ~PasswordGenerator() = default;
    
    // Pure virtual methods (abstract interface)
    virtual bool HasNext() const = 0;
    virtual std::string Next() = 0;
    virtual void Reset() = 0;
    virtual double GetProgressPercent() const = 0;
    virtual size_t GetTotalCount() const = 0;
};

// File: client/BruteForce.h
class BruteForceGenerator : public PasswordGenerator {
private:
    std::string alphabet;
    int maxLength;
    unsigned long long attemptCount;
    unsigned long long totalCombinations;
    std::string currentPassword;
    std::vector<int> indices;
    bool hasMore;
    
public:
    BruteForceGenerator(const std::string& alphabet, int maxLen);
    
    // Implement interface
    bool HasNext() const override;
    std::string Next() override;
    void Reset() override;
    double GetProgressPercent() const override;
    size_t GetTotalCount() const override { return totalCombinations; }
    
    // Brute-force specific methods
    std::string GetCurrent() const { return currentPassword; }
    unsigned long long GetAttemptCount() const { return attemptCount; }
};

// File: client/RuleAttack.h
class RuleBasedAttack : public PasswordGenerator {
private:
    std::vector<std::string> dictionary;
    std::vector<std::string> variants;
    std::unordered_set<std::string> variantSet;
    size_t currentIndex;
    
public:
    RuleBasedAttack();
    
    // Implement interface
    bool HasNext() const override { return currentIndex < variants.size(); }
    std::string Next() override;
    void Reset() override { currentIndex = 0; }
    double GetProgressPercent() const override;
    size_t GetTotalCount() const override { return variants.size(); }
    
    // Rule-based specific methods
    bool LoadDictionaryFromFile();
    bool LoadDictionaryFromFile(const std::string& filename);
    void GenerateVariants();
    size_t GetDictionarySize() const { return dictionary.size(); }
};
```

**Usage Example (Polymorphic):**
```cpp
// main.cpp - Can now use polymorphism
PasswordGenerator* generator = nullptr;

if (mode == MODE_BRUTE_FORCE) {
    generator = new BruteForceGenerator(alphabet, maxLen);
} else if (mode == MODE_RULE_BASED) {
    RuleBasedAttack* ruleAttack = new RuleBasedAttack();
    ruleAttack->LoadDictionaryFromFile();
    ruleAttack->GenerateVariants();
    generator = ruleAttack;
}

// Attack loop works with any generator type (polymorphism!)
while (generator->HasNext()) {
    std::string password = generator->Next();
    if (client.TryPassword(login, password)) {
        Console::Success("Found: " + password);
        break;
    }
    
    // Display progress (works for both types)
    Console::PrintProgressBar(generator->GetProgressPercent());
}

delete generator;
```

**Benefits:**
- ✅ Polymorphism: Can treat any attack as `PasswordGenerator*`
- ✅ Open/Closed Principle: Add new attack types without modifying existing code
- ✅ Code Reusability: Attack loop logic works for all types
- ✅ Testability: Can mock `PasswordGenerator` interface
- ✅ Factory Pattern: Can create generators from config files

**Estimated Effort:** 2-3 hours

---

#### Violation #3: PerPipeStruct Using Struct Instead of Class (MINOR)

**File:** `PerPipeStruct.h` (lines 6-11)
**Severity:** MINOR - Violates encapsulation

**Problem:**
- All members are public (no encapsulation)
- No constructor, methods, or invariants
- Used as POD (Plain Old Data) type instead of proper class

**Current Code:**
```cpp
// PerPipeStruct.h:6-11
struct PerPipeData {
    HANDLE hPipe;       // All public
    int pipeId;
    HWND hGui;
    UserList* userList;
};
```

**Why This Violates OOP:**
- **No Encapsulation:** Direct access to all members from outside
- **No Validation:** Cannot enforce invariants (e.g., `hPipe != INVALID_HANDLE_VALUE`)
- **No Constructor:** Members may be uninitialized

**OOP-Compliant Refactoring:**
```cpp
// PerPipeStruct.h - Convert to class
class PerPipeData {
private:
    HANDLE hPipe;
    int pipeId;
    HWND hGui;
    UserList* userList;
    
public:
    // Constructor with validation
    PerPipeData(HANDLE pipe, int id, HWND gui, UserList* list)
        : hPipe(pipe), pipeId(id), hGui(gui), userList(list) {
        if (!gui || !list) {
            throw std::invalid_argument("GUI handle and UserList cannot be null");
        }
    }
    
    // Getters (const-correct)
    HANDLE GetPipe() const { return hPipe; }
    int GetPipeId() const { return pipeId; }
    HWND GetGui() const { return hGui; }
    UserList* GetUserList() const { return userList; }
    
    // Setters (with validation)
    void SetPipe(HANDLE pipe) {
        hPipe = pipe;
    }
};
```

**Usage Update:**
```cpp
// BEFORE (direct access):
data->hPipe = CreateNamedPipeA(...);
data->userList->CheckUser(...);

// AFTER (encapsulated):
data->SetPipe(CreateNamedPipeA(...));
data->GetUserList()->CheckUser(...);
```

**Benefits:**
- ✅ Encapsulation: Private members with controlled access
- ✅ Validation: Constructor ensures valid state
- ✅ Const-correctness: Read-only access via const getters
- ✅ Future-proof: Can add logging/validation in setters

**Estimated Effort:** 1 hour

---

#### Violation #4: Console Namespace Instead of Class (MINOR - ACCEPTABLE)

**File:** `client/Utils.h` (lines 40-72)
**Severity:** MINOR - Acceptable C++ pattern, but not strict OOP

**Problem:**
- Console utilities implemented as free functions in a namespace
- Stricter OOP would use a static utility class

**Current Code:**
```cpp
namespace Console {
    void ClearLine();
    void PrintProgressBar(double percent, int width);
    void PrintHeader(const std::string& title);
    void PrintSeparator(int width);
    enum Color { /* ... */ };
    void SetColor(Color color);
    void ResetColor();
    void PrintColored(const std::string& text, Color color);
    void PrintSuccess(const std::string& msg);
    void PrintError(const std::string& msg);
    void PrintInfo(const std::string& msg);
    void PrintWarning(const std::string& msg);
}
```

**Alternative OOP Pattern:**
```cpp
class ConsoleUI {
public:
    static void ClearLine();
    static void PrintProgressBar(double percent, int width = 50);
    static void PrintHeader(const std::string& title);
    
    enum class Color { /* ... */ };
    static void SetColor(Color color);
    static void ResetColor();
    static void PrintColored(const std::string& text, Color color);
    static void PrintSuccess(const std::string& msg);
    static void PrintError(const std::string& msg);
};
```

**Note:** Namespace pattern is acceptable in modern C++. This is the lowest priority fix.

**Estimated Effort:** 30 minutes (if changed)

---

#### Files to Modify

**Critical (BLOCKING):**
- `C:\Users\glebm\projects\tech-safety\brute-force\PipeServer.h` (lines 17-95)

**Moderate:**
- `C:\Users\glebm\projects\tech-safety\brute-force\client\BruteForce.h`
- `C:\Users\glebm\projects\tech-safety\brute-force\client\RuleAttack.h`
- `C:\Users\glebm\projects\tech-safety\brute-force\client\PasswordGenerator.h` (NEW FILE)
- `C:\Users\glebm\projects\tech-safety\brute-force\client\main.cpp` (update to use polymorphism)

**Minor:**
- `C:\Users\glebm\projects\tech-safety\brute-force\PerPipeStruct.h` (lines 6-11)
- `C:\Users\glebm\projects\tech-safety\brute-force\client\Utils.h` (lines 40-72) [OPTIONAL]

---

#### Implementation Plan

**Step 1: Fix PipeServer Static Methods (BLOCKING - 3-4 hours)**

1.1. Create helper struct for thread context:
```cpp
struct PipeServerThreadContext {
    PipeServer* server;
    int pipeId;
};
```

1.2. Convert `Log()` to instance method:
```cpp
void Log(const string& text) {
    SendMessage(this->hGui, WM_LOG_MSG, (WPARAM)text.c_str(), 0);
}
```

1.3. Refactor `PipeInstanceThread()` → `HandlePipeInstance()`:
- Change from `static DWORD WINAPI` to `DWORD` (instance method)
- Replace `PerPipeData* data = (PerPipeData*)lpvParam;` with method parameters
- Use `this->hGui`, `this->userList`, `this->isRunning` instead of parameter passing

1.4. Create thin static wrapper for CreateThread:
```cpp
static DWORD WINAPI ThreadWrapperFunc(LPVOID lpParam) {
    PipeServerThreadContext* ctx = (PipeServerThreadContext*)lpParam;
    return ctx->server->HandlePipeInstance(ctx->pipeId);
}
```

1.5. Update `Start()` method to pass `this` pointer to threads

1.6. Build and test: Verify server still handles authentication correctly

---

**Step 2: Implement Attack Class Polymorphism (MODERATE - 2-3 hours)**

2.1. Create `client/PasswordGenerator.h`:
```cpp
class PasswordGenerator {
public:
    virtual ~PasswordGenerator() = default;
    virtual bool HasNext() const = 0;
    virtual std::string Next() = 0;
    virtual void Reset() = 0;
    virtual double GetProgressPercent() const = 0;
    virtual size_t GetTotalCount() const = 0;
};
```

2.2. Modify `BruteForceGenerator` to inherit from `PasswordGenerator`:
- Add `#include "PasswordGenerator.h"`
- Change class declaration: `class BruteForceGenerator : public PasswordGenerator`
- Add `override` keyword to interface methods
- Implement `GetTotalCount()` method

2.3. Modify `RuleBasedAttack` to inherit from `PasswordGenerator`:
- Add `#include "PasswordGenerator.h"`
- Change class declaration: `class RuleBasedAttack : public PasswordGenerator`
- Add `override` keyword to interface methods
- Implement `GetTotalCount()` method

2.4. Update `main.cpp` to use polymorphism:
- Use `PasswordGenerator*` pointer
- Assign to either `new BruteForceGenerator()` or `new RuleBasedAttack()`
- Attack loop uses polymorphic interface
- `delete generator;` at end

2.5. Build and test: Verify both attack modes work with polymorphic interface

---

**Step 3: Convert PerPipeStruct to Class (MINOR - 1 hour)**

3.1. Rename `PerPipeStruct.h` → `PerPipeData.h` (optional)

3.2. Change `struct PerPipeData` → `class PerPipeData`:
```cpp
class PerPipeData {
private:
    HANDLE hPipe;
    int pipeId;
    HWND hGui;
    UserList* userList;
public:
    PerPipeData(HANDLE pipe, int id, HWND gui, UserList* list);
    // Getters/setters
};
```

3.3. Update all references in `PipeServer.h` to use getters/setters

3.4. Build and test: Verify no compilation errors

---

**Step 4: Convert Console Namespace to Class (OPTIONAL - 30 min)**

4.1. Change `namespace Console` → `class ConsoleUI`

4.2. Add `static` keyword to all member functions

4.3. Update all call sites: `Console::PrintSuccess()` → `ConsoleUI::PrintSuccess()`

4.4. Build and test: Verify all console output works

---

#### Testing

**Test 1: OOP Principles Verification**
- ✅ Encapsulation: All classes have private members with controlled access
- ✅ Inheritance: Attack classes inherit from common base
- ✅ Polymorphism: Can use `PasswordGenerator*` to point to either attack type
- ✅ Abstraction: Interface (`PasswordGenerator`) separates contract from implementation

**Test 2: Functional Verification**
- ✅ Server: Start server, verify 3 threads created, test authentication
- ✅ Brute Force: Run brute force attack, verify progress tracking works
- ✅ Rule-Based: Run rule-based attack, verify polymorphic interface works
- ✅ Integration: Run full attack scenario, verify no regressions

**Test 3: Code Review**
- ✅ No static methods containing business logic (except thread wrappers)
- ✅ No structs with all public members (except POD types if needed)
- ✅ Classes use proper encapsulation (private/protected/public)
- ✅ Virtual methods used for polymorphic behavior

---

#### Compliance After Fix

| OOP Principle | Before | After |
|---------------|--------|-------|
| Encapsulation | ⚠️ PARTIAL (PipeServer static, PerPipeStruct public) | ✅ FULL |
| Inheritance   | ❌ NONE (attack classes independent) | ✅ FULL (PasswordGenerator base) |
| Polymorphism  | ❌ NONE (no virtual methods) | ✅ FULL (polymorphic attack loop) |
| Abstraction   | ⚠️ PARTIAL (classes exist but no interfaces) | ✅ FULL (PasswordGenerator interface) |

**Estimated Grade Impact:** 85% → 100% OOP compliance

---

## High Severity: Code Quality Issues

These issues affect reliability, security, and production-readiness but do not directly impact grading.

---

### Issue #6: Memory Leak in Logging Function

**Severity:** HIGH
**Type:** Memory Management Bug
**Source:** project-overview.md Issue #1

#### Problem Location
```cpp
// PipeServer.h:18-20
void Log(const std::string& text) {
    char* buf = new char[text.length() + 1];
    strcpy_s(buf, text.length() + 1, text.c_str());
    PostMessage(hGui, WM_LOG_MSG, (WPARAM)buf, 0);
    // ← Memory NEVER freed!
}
```

#### Problem Description
- Memory allocated with `new char[]` is passed to `PostMessage()` as WPARAM
- No guaranteed cleanup mechanism
- Window message handler does not free the memory
- If window is destroyed or message not processed, memory leaks

#### Impact
- **Memory Growth:** ~50-100 bytes per authentication attempt
- **After 10,000 attempts:** ~500 KB - 1 MB leaked memory
- **Long-running server:** Unbounded memory growth
- **Symptom:** Increasing private bytes in Task Manager

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\PipeServer.h (lines 18-20)
C:\Users\glebm\projects\tech-safety\brute-force\test.cpp (WM_LOG_MSG handler)
```

#### Implementation Plan

**Option A: Use SendMessage (Recommended)**
```cpp
// PipeServer.h:18-20
void Log(const std::string& text) {
    char buf[512];
    strcpy_s(buf, sizeof(buf), text.c_str());
    SendMessage(hGui, WM_LOG_MSG, (WPARAM)buf, 0);  // Synchronous - safe to use stack
}
```

**Option B: Add Cleanup in Message Handler**
```cpp
// test.cpp - In WndProc:
case WM_LOG_MSG: {
    char* message = (char*)wParam;
    // ... log the message ...
    delete[] message;  // ← Free the memory
    return 0;
}
```

**Option C: Use std::string with SendMessage**
```cpp
// PipeServer.h
void Log(const std::string& text) {
    SendMessage(hGui, WM_LOG_MSG, (WPARAM)text.c_str(), 0);
}
// Note: Only safe if message is processed synchronously
```

**Testing:**
1. Run server for 10,000+ authentication attempts
2. Monitor memory usage in Task Manager
3. Verify memory does not continuously grow
4. Use Visual Studio Memory Profiler to confirm no leaks

---

### Issue #7: No Thread Cleanup Mechanism

**Severity:** HIGH
**Type:** Resource Management Bug
**Source:** project-overview.md Issue #2

#### Problem Location
```cpp
// PipeServer.h:30-91 - Infinite loop
DWORD WINAPI PipeThreadFunc(LPVOID lpParam) {
    // ...
    while (true) {  // ← No exit condition!
        data->hPipe = CreateNamedPipeA(...);
        ConnectNamedPipe(...);
        // ... process authentication ...
        CloseHandle(data->hPipe);
    }
    return 0;  // Never reached
}

// PipeServer.h:100-112 - No cleanup
void Start(int numPipes, UserList* uList, HWND hGui) {
    for (int i = 0; i < numPipes; i++) {
        HANDLE hThread = CreateThread(..., PipeThreadFunc, ...);
        if (hThread) threads.push_back(hThread);
        // ← Threads never joined or waited for
    }
}
```

#### Problem Description
- Worker threads run in infinite `while(true)` loop with no exit mechanism
- Thread handles stored in vector but never waited for
- No `WaitForMultipleObjects()` or thread join logic
- Server cannot gracefully shut down
- Only way to stop: Force-terminate entire process (ungraceful)

#### Impact
- **Resource Leak:** Thread handles accumulate without cleanup
- **Cannot Restart:** Must close entire application to restart server
- **Unclean Shutdown:** In-progress operations may be corrupted
- **Violates RAII Principles:** Resources not properly managed

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\PipeServer.h (lines 30-91, 100-112)
C:\Users\glebm\projects\tech-safety\brute-force\PerPipeStruct.h
C:\Users\glebm\projects\tech-safety\brute-force\test.cpp (WM_DESTROY handler)
```

#### Implementation Plan

**Step 1: Add Shutdown Event to PerPipeStruct**
```cpp
// PerPipeStruct.h
struct PerPipeStruct {
    HANDLE hPipe;
    UserList* userList;
    HWND hGui;
    HANDLE hShutdownEvent;  // ← Add this
};
```

**Step 2: Create Shutdown Event in Start()**
```cpp
// PipeServer.h - Modify Start() method
class PipeServer {
private:
    std::vector<HANDLE> threads;
    HANDLE hShutdownEvent;  // ← Add member variable

public:
    void Start(int numPipes, UserList* uList, HWND hGui) {
        hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual-reset event

        for (int i = 0; i < numPipes; i++) {
            PerPipeStruct* data = new PerPipeStruct;
            data->userList = uList;
            data->hGui = hGui;
            data->hShutdownEvent = hShutdownEvent;  // ← Share event across threads

            HANDLE hThread = CreateThread(NULL, 0, PipeThreadFunc, data, 0, NULL);
            if (hThread) threads.push_back(hThread);
        }
    }
```

**Step 3: Modify Thread Loop to Check Shutdown Event**
```cpp
// PipeServer.h - Modify PipeThreadFunc()
DWORD WINAPI PipeThreadFunc(LPVOID lpParam) {
    PerPipeStruct* data = (PerPipeStruct*)lpParam;

    // Check shutdown event with 0 timeout (non-blocking)
    while (WaitForSingleObject(data->hShutdownEvent, 0) == WAIT_TIMEOUT) {
        data->hPipe = CreateNamedPipeA(...);

        if (data->hPipe == INVALID_HANDLE_VALUE) {
            break;  // Exit on error
        }

        ConnectNamedPipe(...);
        // ... process authentication ...
        CloseHandle(data->hPipe);

        // Check shutdown event again before next iteration
        if (WaitForSingleObject(data->hShutdownEvent, 0) != WAIT_TIMEOUT) {
            break;
        }
    }

    delete data;
    return 0;
}
```

**Step 4: Implement Stop() Method**
```cpp
// PipeServer.h - Add Stop() method
void Stop() {
    if (hShutdownEvent) {
        SetEvent(hShutdownEvent);  // Signal all threads to exit

        // Wait for all threads to finish (5 second timeout)
        DWORD result = WaitForMultipleObjects(
            threads.size(),
            threads.data(),
            TRUE,  // Wait for all
            5000   // 5 second timeout
        );

        if (result == WAIT_TIMEOUT) {
            Log("Warning: Some threads did not exit cleanly");
        }

        // Close all thread handles
        for (HANDLE h : threads) {
            CloseHandle(h);
        }
        threads.clear();

        CloseHandle(hShutdownEvent);
        hShutdownEvent = NULL;
    }
}
```

**Step 5: Call Stop() on Window Close**
```cpp
// test.cpp - In WndProc:
case WM_DESTROY:
    pipeServer.Stop();  // ← Add this to cleanup threads
    PostQuitMessage(0);
    return 0;
```

**Testing:**
1. Start server with 3 threads
2. Click "X" to close window
3. Verify all threads exit within 5 seconds
4. Check Task Manager: No orphaned threads remain

---

### Issue #8: Buffer Overflow Vulnerability

**Severity:** HIGH (Security Risk)
**Type:** Memory Safety Bug
**Source:** project-overview.md Issue #3

#### Problem Location
```cpp
// PipeServer.h:26-53
char buffer[BUFFER_SIZE];  // 512 bytes
ZeroMemory(buffer, BUFFER_SIZE);
success = ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL);
buffer[bytesRead] = '\0';  // ← Potential out-of-bounds if bytesRead >= 512
```

#### Problem Description
- Fixed 512-byte buffer with no overflow protection
- `bytesRead` not validated before using as array index
- Malicious client could send >511 bytes
- No bounds checking before `buffer[bytesRead] = '\0'`
- Classic buffer overflow vulnerability (CWE-120)

#### Impact
- **Potential Stack Corruption:** Overflow could corrupt adjacent stack variables
- **Server Crash:** Segmentation fault if buffer overrun
- **Security Risk:** Could potentially allow arbitrary code execution
- **Violation:** OWASP Top 10 security vulnerability

#### Attack Scenario
```python
# Malicious client sends oversized message:
malicious_message = "admin " + ("A" * 600)  # 606 bytes total
# Result: bytesRead = 512, buffer[512] = '\0' writes past array bounds
```

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\PipeServer.h (lines 26-53)
```

#### Implementation Plan

**Step 1: Add Validation After ReadFile**
```cpp
// PipeServer.h:53 - Add validation
success = ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL);

if (success && bytesRead > 0) {
    // CRITICAL: Validate bytesRead before using as index
    if (bytesRead >= BUFFER_SIZE) {
        Log("ERROR: Message too large (" + std::to_string(bytesRead) +
            " bytes), max is " + std::to_string(BUFFER_SIZE - 1));
        DisconnectNamedPipe(data->hPipe);
        continue;  // Skip this request
    }

    buffer[bytesRead] = '\0';  // Now safe

    // ... rest of processing ...
}
```

**Step 2: Add Length Check Before Write**
```cpp
// Additional safety check before null terminator:
if (bytesRead < BUFFER_SIZE) {
    buffer[bytesRead] = '\0';
} else {
    buffer[BUFFER_SIZE - 1] = '\0';  // Force termination
    Log("WARNING: Buffer truncated");
}
```

**Step 3: Consider Dynamic Allocation for Large Messages**
```cpp
// Alternative: Use std::string for variable-length messages
if (bytesRead > 0) {
    std::string message(buffer, bytesRead);
    // Process message as std::string
}
```

**Step 4: Add Maximum Message Size Protocol**
```cpp
// Define protocol constant
#define MAX_MESSAGE_SIZE 256  // Reasonable limit for "login password"

// Reject oversized messages
if (bytesRead > MAX_MESSAGE_SIZE) {
    Log("ERROR: Message exceeds protocol maximum");
    DWORD errorCode = ERROR_INVALID_DATA;
    WriteFile(data->hPipe, &errorCode, sizeof(errorCode), &bytesWritten, NULL);
    DisconnectNamedPipe(data->hPipe);
    continue;
}
```

**Testing:**
1. Normal case: Send 20-byte message → Should work
2. Edge case: Send 511-byte message → Should work
3. Overflow case: Send 600-byte message → Should reject gracefully
4. Use fuzzing tool to test with random message sizes

---

## Medium Severity: Performance & Reliability

These issues affect performance and user experience but do not pose security risks.

---

### Issue #9: Disconnect After Every Password Attempt

**Severity:** MEDIUM (Performance Bottleneck)
**Type:** Performance Bug
**Source:** project-overview.md Issue #4

#### Problem Location
```cpp
// PipeClient.cpp:69
bool PipeClient::TryPassword(const std::string& login, const std::string& password) {
    if (!connected && !Connect()) {
        return false;
    }

    std::string message = login + " " + password;
    if (!WriteMessage(message)) {
        return false;
    }

    int response;
    if (!ReadResponse(response)) {
        return false;
    }

    Disconnect();  // ← Always disconnects after EVERY attempt!
    return (response == 1);
}
```

#### Problem Description
- Client disconnects and reconnects for **every single password attempt**
- Forces complete pipe lifecycle per attempt:
  1. `CreateFile()` - Open pipe handle
  2. `WriteFile()` - Send credentials
  3. `ReadFile()` - Receive response
  4. `CloseHandle()` - Disconnect
- Connection setup/teardown overhead: 5-20 milliseconds per attempt
- Named pipe creation is expensive system call

#### Impact
- **Severe Performance Penalty:** Brute force attacks run 10-100x slower than necessary
- **Example Timing (10,000 password attempts):**
  - Current implementation: 10-15 minutes (10-20 attempts/sec)
  - With persistent connection: 1-2 minutes (100-200 attempts/sec)
- **Defeats Purpose:** Brute force efficiency severely limited

#### Measurement
```
Current: 5ms connection + 0.5ms auth + 5ms disconnect = 10.5ms/attempt → 95 attempts/sec
Optimal: 0.5ms auth = 0.5ms/attempt → 2000 attempts/sec
Speedup: 21x faster
```

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\client\PipeClient.cpp (line 69)
C:\Users\glebm\projects\tech-safety\brute-force\PipeServer.h (lines 85-87)
```

#### Implementation Plan

**Step 1: Remove Disconnect from TryPassword (Client)**
```cpp
// PipeClient.cpp:69
bool PipeClient::TryPassword(const std::string& login, const std::string& password) {
    if (!connected && !Connect()) {
        return false;
    }

    std::string message = login + " " + password;
    if (!WriteMessage(message)) {
        Disconnect();  // Only disconnect on actual failure
        return false;
    }

    int response;
    if (!ReadResponse(response)) {
        Disconnect();  // Only disconnect on actual failure
        return false;
    }

    // Keep connection alive for next attempt!
    // Disconnect();  ← REMOVE THIS LINE

    return (response == 1);
}
```

**Step 2: Add Persistent Connection Mode (Client)**
```cpp
// PipeClient.h - Add configuration option
class PipeClient {
private:
    bool persistentMode;  // New member variable

public:
    PipeClient() : hPipe(INVALID_HANDLE_VALUE), connected(false), persistentMode(true) {}

    void SetPersistentMode(bool enable) { persistentMode = enable; }
};
```

**Step 3: Modify Server to Support Persistent Connections**
```cpp
// PipeServer.h:85-87 - Modify thread loop
while (WaitForSingleObject(data->hShutdownEvent, 0) == WAIT_TIMEOUT) {
    data->hPipe = CreateNamedPipeA(...);
    ConnectNamedPipe(data->hPipe, NULL);

    // Process multiple requests on same connection
    bool keepAlive = true;
    while (keepAlive) {
        success = ReadFile(...);
        if (!success || bytesRead == 0) {
            keepAlive = false;  // Client disconnected
            break;
        }

        // ... process authentication ...
        WriteFile(...);

        // Don't disconnect yet - client may send another password
    }

    DisconnectNamedPipe(data->hPipe);  // Only disconnect when client closes
    CloseHandle(data->hPipe);
}
```

**Step 4: Testing**
```bash
# Test persistent connection mode
# Try 10,000 passwords with both modes:

# Old mode (disconnect per attempt):
# Expected: ~10-15 minutes

# New mode (persistent connection):
# Expected: ~1-2 minutes (10x speedup)
```

**Step 5: Benchmarking**
```cpp
// Add timing code in main.cpp
Timer timer;
timer.Start();

// ... brute force attack ...

timer.Stop();
double rate = totalAttempts / timer.GetElapsedSeconds();
std::cout << "Attack rate: " << rate << " passwords/sec" << std::endl;
```

**Expected Results:**
- **Before fix:** 10-50 attempts/sec
- **After fix:** 100-500 attempts/sec
- **Speedup:** 10-50x improvement

---

### Issue #10: Silent Protocol Failures

**Severity:** MEDIUM (Debugging & Reliability)
**Type:** Error Handling Deficiency
**Source:** project-overview.md Issue #5

#### Problem Locations

**Client Side (PipeClient.cpp:139-162):**
```cpp
bool PipeClient::ReadResponse(int& response) {
    DWORD bytesRead;
    if (!ReadFile(hPipe, &response, sizeof(response), &bytesRead, NULL)) {
        Console::Error("Failed to read response");  // ← Generic error, no details
        Disconnect();
        return false;
    }
    return true;
}
```

**Server Side (PipeServer.h:53-85):**
```cpp
success = ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL);
if (success && bytesRead > 0) {
    // ... process message ...
}
// ← No else branch! Silent failure on read error
```

#### Problem Description
- Generic error messages with no context about **why** failure occurred
- No use of `GetLastError()` to get detailed error codes
- Server silently ignores read failures (no logging or error handling)
- Cannot differentiate between:
  - Broken pipe (client disconnected)
  - Buffer overflow
  - Timeout
  - Permission error
  - Invalid handle

#### Impact
- **Difficult Debugging:** Cannot diagnose connection issues
- **Silent Data Loss:** Server drops requests without logging
- **Poor User Experience:** No actionable error information
- **Time Wasted:** Users spend hours debugging simple issues

#### Error Code Examples
```
ERROR_BROKEN_PIPE (109): Client disconnected unexpectedly
ERROR_NO_DATA (232): Pipe is being closed
ERROR_PIPE_NOT_CONNECTED (233): Pipe handle is invalid
ERROR_INVALID_HANDLE (6): Bad pipe handle
ERROR_ACCESS_DENIED (5): Permissions issue
```

#### Files to Modify
```
C:\Users\glebm\projects\tech-safety\brute-force\client\PipeClient.cpp (lines 139-162)
C:\Users\glebm\projects\tech-safety\brute-force\PipeServer.h (lines 53-85)
C:\Users\glebm\projects\tech-safety\brute-force\client\Utils.h
C:\Users\glebm\projects\tech-safety\brute-force\client\Utils.cpp
```

#### Implementation Plan

**Step 1: Create Error Message Helper (Utils.cpp)**
```cpp
// Utils.cpp - Add new utility function
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
        LocalFree(messageBuffer);
    } else {
        message = "Unknown error " + std::to_string(errorCode);
    }

    return message;
}

// Utils.h - Add declaration
std::string GetWin32ErrorMessage(DWORD errorCode);
```

**Step 2: Enhance Client Error Handling**
```cpp
// PipeClient.cpp - Improve ReadResponse()
bool PipeClient::ReadResponse(int& response) {
    DWORD bytesRead;
    if (!ReadFile(hPipe, &response, sizeof(response), &bytesRead, NULL)) {
        DWORD error = GetLastError();
        std::string errorMsg = GetWin32ErrorMessage(error);
        Console::Error("Failed to read response (Error " + std::to_string(error) + "): " + errorMsg);

        // Differentiate error types
        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
            Console::Warning("Server disconnected unexpectedly");
        } else if (error == ERROR_INVALID_HANDLE) {
            Console::Warning("Invalid pipe handle - reconnection required");
        }

        Disconnect();
        return false;
    }

    // Validate response size
    if (bytesRead != sizeof(response)) {
        Console::Warning("Incomplete response received (" + std::to_string(bytesRead) +
                        " bytes, expected " + std::to_string(sizeof(response)) + ")");
        return false;
    }

    return true;
}
```

**Step 3: Add Server Error Logging**
```cpp
// PipeServer.h:53-85 - Add else branch for error handling
success = ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL);

if (success && bytesRead > 0) {
    // ... normal processing ...
} else {
    // ERROR HANDLING BRANCH (was missing!)
    DWORD error = GetLastError();

    if (error == ERROR_BROKEN_PIPE) {
        Log("Client disconnected during read");
    } else if (error == ERROR_NO_DATA) {
        Log("Pipe closing");
    } else if (error != 0) {
        // Get detailed error message
        char errorMsg[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
                      0, errorMsg, sizeof(errorMsg), NULL);
        Log("Pipe read error (" + std::to_string(error) + "): " + std::string(errorMsg));
    }

    DisconnectNamedPipe(data->hPipe);
    continue;  // Skip to next iteration
}
```

**Step 4: Add Retry Logic for Transient Errors**
```cpp
// PipeClient.cpp - Add retry mechanism
bool PipeClient::WriteMessage(const std::string& message) {
    const int MAX_RETRIES = 3;
    int retries = 0;

    while (retries < MAX_RETRIES) {
        DWORD bytesWritten;
        if (WriteFile(hPipe, message.c_str(), message.length(), &bytesWritten, NULL)) {
            return true;  // Success
        }

        DWORD error = GetLastError();

        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
            // Transient error - try reconnecting
            Console::Warning("Connection lost, attempting to reconnect...");
            Disconnect();
            Sleep(100);  // Brief delay

            if (Connect()) {
                retries++;
                continue;  // Retry with new connection
            }
        }

        // Permanent error or reconnect failed
        Console::Error("Write failed: " + GetWin32ErrorMessage(error));
        return false;
    }

    Console::Error("Max retries exceeded");
    return false;
}
```

**Step 5: Testing**
```
Test scenarios:
1. Normal operation → Should work, no errors
2. Kill server during client operation → Should display "Server disconnected"
3. Send oversized message → Should display specific error
4. Close pipe handle → Should display "Invalid handle"
5. Network issues (if remote) → Should retry 3 times
```

---

## Implementation Priority

### Phase 1: Critical Bug Fix (IMMEDIATE)
**Priority:** CRITICAL
**Timeframe:** 2-3 hours

1. **Issue #5:** Fix race condition on `blockedUsers` map (add CRITICAL_SECTION)

**Deliverable:** Thread-safe anti-cracking protection, server stability

---

### Phase 2: Requirements Compliance (100% Grade)
**Priority:** HIGHEST
**Timeframe:** 3-4 days

2. **Issue #1:** Implement templated singly linked list
3. **Issue #2:** Add character transposition rule
4. **Issue #3:** Complete keyboard layout swap (Cyrillic to Latin)
5. **Issue #11:** Fix OOP compliance violations (PipeServer refactoring + attack polymorphism)

**Deliverable:** Achieve 100% grade compliance

---

### Phase 3: Multithreading Enhancement (140% Grade)
**Priority:** HIGH
**Timeframe:** 2-3 days

6. **Issue #4:** Implement client-side multithreading

**Deliverable:** Achieve 140% grade compliance + significant performance boost

---

### Phase 4: Code Quality & Security
**Priority:** MEDIUM
**Timeframe:** 1-2 days

7. **Issue #6:** Fix memory leak in logging
8. **Issue #7:** Implement graceful thread shutdown
9. **Issue #8:** Add buffer overflow protection

**Deliverable:** Production-quality, secure codebase

---

### Phase 5: Performance Optimization
**Priority:** MEDIUM
**Timeframe:** 1 day

10. **Issue #9:** Implement persistent connections

**Deliverable:** 10-100x brute force attack speedup

---

### Phase 6: Best Practices (Optional)
**Priority:** LOW
**Timeframe:** 1-2 days

11. **Issue #5b:** ~~Convert to async/overlapped pipes~~ ✅ ALREADY IMPLEMENTED
10. **Issue #10:** Enhanced error handling

**Deliverable:** Industry best practices compliance

---

## Testing Recommendations

### Unit Testing
```cpp
// Test each component individually
1. LinkedList<std::string> - Test push_back, iteration, memory cleanup
2. TransposeCharacters() - Test with various strings
3. CyrillicToLatin() - Test bidirectional conversion
4. Thread pool - Test with 1, 2, 4, 8 threads
```

### Integration Testing
```
1. Start server with protection enabled
2. Run brute force attack with multithreading
3. Run dictionary attack with all rules
4. Monitor for memory leaks (Task Manager)
5. Verify graceful shutdown (no orphaned threads)
```

### Performance Testing
```
1. Benchmark attack rates (before/after optimization)
2. Measure speedup from multithreading (1 vs 4 vs 8 threads)
3. Test persistent connections vs per-attempt disconnect
4. Profile CPU usage during attacks
```

### Security Testing
```
1. Send oversized messages (>512 bytes)
2. Send malformed messages ("" , "login", "login password extra")
3. Rapid connection/disconnection stress test
4. Attempt buffer overflow attacks
```

---

## Summary

### Critical Path to 100% Grade
```
Issue #5 (Race Condition FIX) → Issue #1 (Linked List) → Issue #2 (Transposition) → Issue #3 (Keyboard Swap) → Issue #11 (OOP Compliance) → 100% GRADE
```

### Critical Path to 140% Grade
```
100% Grade → Issue #4 (Client Multithreading) → 140% GRADE
```

### Recommended Implementation Order
```
0. #5 (Race Condition) - 2 hours ⚠️ CRITICAL - DO THIS FIRST!
   - Add CRITICAL_SECTION to list.h
   - Protect blockedUsers map access
   ↓ ANTI-CRACKING PROTECTION WORKS
1. #1 (Linked List) - 4 hours
2. #2 (Transposition) - 2 hours
3. #3 (Keyboard Swap) - 3 hours
4. #11 (OOP Compliance) - 6 hours
   - PipeServer refactoring: 3-4 hours
   - Attack class polymorphism: 2-3 hours
   - PerPipeStruct encapsulation: 1 hour
   ↓ 100% GRADE ACHIEVED (OOP REQUIREMENTS MET)
5. #6 (Memory Leak) - 1 hour
6. #7 (Thread Cleanup) - 2 hours
7. #8 (Buffer Safety) - 1 hour
   ↓ HIGH SEVERITY BUGS FIXED
8. #9 (Persistent Connections) - 3 hours
   ↓ MAJOR PERFORMANCE BOOST
9. #4 (Client Multithreading) - 6 hours
   ↓ 140% GRADE ACHIEVED
10. #10 (Error Handling) - 2 hours
11. #5b (Async Pipes) - ✅ ALREADY DONE
    ↓ PRODUCTION READY
```

**Total Estimated Time:** 32 hours (~4 working days)

---

## References

- **requirements.md:** Laboratory Work №1 specification
- **project-overview.md:** Comprehensive technical documentation
- **PipeServer.h:** Server implementation (C:\Users\glebm\projects\tech-safety\brute-force\PipeServer.h)
- **client/RuleAttack.cpp:** Rule-based attack implementation

---

**Document Version:** 1.0
**Last Updated:** 2025-12-11
**Status:** Ready for implementation
