# Multithreading Implementation Plan for 140% Grade

## Executive Summary

This plan implements multithreading for both client and server components to achieve the 140% grade requirement. The key requirements are:
- **Client**: Auto-detect hardware threads, allow user configuration within bounds, display simple overall stats (Option 1)
- **Server**: Remove hardcoded thread count, add auto-detection and GUI spinner control

## Phase 1: Server Thread Configuration (2-3 hours)

### 1.1 Update ServerConstants.h
**File**: `server/ServerConstants.h`

`★ Insight ─────────────────────────────────────`
The current hardcoded `MAX_CONCURRENT_PIPES = 16` violates OOP's dependency inversion principle. We'll make it configurable at runtime while maintaining backward compatibility.
`─────────────────────────────────────────────────`

```cpp
// Replace line 6
const int MAX_CONCURRENT_PIPES = 16;  // OLD - Hardcoded

// With:
const int DEFAULT_CONCURRENT_PIPES = 0;  // 0 = auto-detect
const int MIN_CONCURRENT_PIPES = 3;      // Per requirements.pdf page 3
const int MAX_CONCURRENT_PIPES = 255;    // Windows named pipe limit
```

### 1.2 Enhance ServerContext.h
**File**: `server/ServerContext.h`

```cpp
// Add to private members:
int configuredThreadCount;

// Add to public:
int GetThreadCount() const { return configuredThreadCount; }
void SetThreadCount(int count);
static int AutoDetectThreadCount();

// Implementation in ServerContext.cpp:
int ServerContext::AutoDetectThreadCount() {
    unsigned int hwThreads = std::thread::hardware_concurrency();
    return (hwThreads == 0) ? MIN_CONCURRENT_PIPES :
           std::min((int)hwThreads, MAX_CONCURRENT_PIPES);
}
```

### 1.3 Add GUI Spinner Control
**File**: `server/test.cpp`

`★ Insight ─────────────────────────────────────`
Windows Common Controls provide a built-in Up-Down (spinner) control that automatically handles numeric validation and range checking - perfect for thread count configuration.
`─────────────────────────────────────────────────`

Add these control IDs (after line 12):
```cpp
#define IDC_THREAD_EDIT 105
#define IDC_THREAD_SPIN 106
```

Update WM_CREATE section:
```cpp
// After Anti-Brute-Force Mode checkbox:
CreateWindow("STATIC", "Threads:", WS_VISIBLE | WS_CHILD,
    330, 12, 50, 20, hwnd, NULL, NULL, NULL);

HWND hEditThreads = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_RIGHT,
    385, 10, 40, 25, hwnd, (HMENU)IDC_THREAD_EDIT, NULL, NULL);

HWND hSpinner = CreateWindowEx(0, UPDOWN_CLASS, NULL,
    WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
    0, 0, 0, 0, hwnd, (HMENU)IDC_THREAD_SPIN, NULL, NULL);

// Configure spinner
SendMessage(hSpinner, UDM_SETBUDDY, (WPARAM)hEditThreads, 0);
SendMessage(hSpinner, UDM_SETRANGE, 0, MAKELONG(MAX_CONCURRENT_PIPES, MIN_CONCURRENT_PIPES));
SendMessage(hSpinner, UDM_SETPOS, 0, AutoDetectThreadCount());
```

Update WM_COMMAND for IDC_START_BTN:
```cpp
// Read thread count from spinner
BOOL success;
int threadCount = GetDlgItemInt(hwnd, IDC_THREAD_EDIT, &success, FALSE);
if (!success) threadCount = context->GetThreadCount();
context->SetThreadCount(threadCount);
context->GetServer().Start(threadCount, &context->GetUsers(), hwnd);
```

### 1.4 Update PipeServer.h Logging
**File**: `server/PipeServer.h`

Enhance Start() method to log actual thread count:
```cpp
Log(hGui, "Server Configuration:");
Log(hGui, "  Thread Count: " + to_string(numPipes));
Log(hGui, "  Named Pipe: " + string(PIPE_NAME));
```

## Phase 2: Client Thread Infrastructure (4-5 hours)

### 2.1 Create ThreadSafePasswordGenerator.h
**New file**: `client/ThreadSafePasswordGenerator.h`

`★ Insight ─────────────────────────────────────`
Using the Adapter pattern, we wrap existing generators to make them thread-safe without modifying their internals. This follows the Open/Closed Principle.
`─────────────────────────────────────────────────`

```cpp
#pragma once
#include <mutex>
#include <memory>
#include "PasswordGenerator.h"

class ThreadSafePasswordGenerator {
private:
    std::unique_ptr<PasswordGenerator> generator;
    std::mutex generatorMutex;

public:
    explicit ThreadSafePasswordGenerator(PasswordGenerator* gen)
        : generator(gen) {}

    std::string GetNextPassword() {
        std::lock_guard<std::mutex> lock(generatorMutex);
        return generator->HasNext() ? generator->Next() : "";
    }

    bool HasMore() const {
        return generator->HasNext();
    }

    size_t GetTotalCount() const {
        return generator->GetTotalCount();
    }

    // Delete copy operations (mutex isn't copyable)
    ThreadSafePasswordGenerator(const ThreadSafePasswordGenerator&) = delete;
    ThreadSafePasswordGenerator& operator=(const ThreadSafePasswordGenerator&) = delete;
};
```

### 2.2 Create AttackContext.h
**New file**: `client/AttackContext.h`

```cpp
#pragma once
#include <atomic>
#include <string>
#include <mutex>

struct AttackContext {
    std::atomic<bool> passwordFound{false};
    std::atomic<unsigned long long> totalAttempts{0};

    std::mutex resultMutex;
    std::string foundLogin;
    std::string foundPassword;

    const std::string targetLogin;

    explicit AttackContext(const std::string& login) : targetLogin(login) {}

    void MarkFound(const std::string& login, const std::string& password) {
        std::lock_guard<std::mutex> lock(resultMutex);
        foundLogin = login;
        foundPassword = password;
        passwordFound.store(true, std::memory_order_release);
    }

    bool IsFound() const {
        return passwordFound.load(std::memory_order_acquire);
    }

    void IncrementAttempts() {
        totalAttempts.fetch_add(1, std::memory_order_relaxed);
    }
};
```

### 2.3 Create AttackWorker.h
**New file**: `client/AttackWorker.h`

`★ Insight ─────────────────────────────────────`
Each worker gets its own PipeClient instance to avoid connection sharing issues. This leverages the server's ability to handle multiple concurrent connections.
`─────────────────────────────────────────────────`

```cpp
#pragma once
#include "ThreadSafePasswordGenerator.h"
#include "AttackContext.h"
#include "PipeClient.h"
#include <thread>

class AttackWorker {
public:
    static void WorkerThreadFunc(
        ThreadSafePasswordGenerator* generator,
        AttackContext* context,
        int threadId)
    {
        PipeClient client;

        if (!client.Connect()) {
            return; // Connection failed
        }

        while (!context->IsFound()) {
            std::string password = generator->GetNextPassword();
            if (password.empty()) break; // No more passwords

            context->IncrementAttempts();

            if (client.TryPassword(context->targetLogin, password)) {
                context->MarkFound(context->targetLogin, password);
                break;
            }

            // Check periodically for early termination
            if (context->IsFound()) break;
        }

        client.Disconnect();
    }
};
```

### 2.4 Update ClientConstants.h
**File**: `client/ClientConstants.h`

```cpp
// Add threading constants:
const int MIN_THREAD_COUNT = 1;
const int MAX_THREAD_COUNT = 16;  // Limited by server
const int DEFAULT_THREAD_COUNT = 0;  // 0 = auto-detect
```

## Phase 3: Client Attack Mode Updates (3-4 hours)

### 3.1 Modify ModeBruteForce()
**File**: `client/main.cpp`

Replace existing function with multithreaded version:

```cpp
void ModeBruteForce() {
    // ... existing alphabet selection code ...

    // NEW: Thread configuration
    int threadCount = 0;
    unsigned int hwThreads = std::thread::hardware_concurrency();
    int maxAllowed = std::min((int)hwThreads, ClientConfig::MAX_THREAD_COUNT);

    std::cout << "\nThread Configuration:\n";
    std::cout << "  Hardware threads: " << hwThreads << "\n";
    std::cout << "  Max allowed: " << maxAllowed << "\n";
    std::cout << "Enter thread count (1-" << maxAllowed << ", 0=auto): ";
    std::cin >> threadCount;

    if (threadCount == 0) threadCount = maxAllowed;
    if (threadCount < 1 || threadCount > maxAllowed) {
        Console::PrintError("Invalid thread count!");
        return;
    }

    // Create thread-safe generator
    ThreadSafePasswordGenerator generator(new BruteForceGenerator(alphabet, maxLength));
    AttackContext context(targetLogin);

    // Launch worker threads
    std::vector<std::thread> workers;
    for (int i = 0; i < threadCount; i++) {
        workers.emplace_back(AttackWorker::WorkerThreadFunc,
                            &generator, &context, i + 1);
    }

    // Progress monitoring (Option 1: Simple stats only)
    Timer timer;
    timer.Start();

    while (!context.IsFound() && generator.HasMore()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        Console::ClearLine();
        std::cout << "Attempts: " << context.GetAttempts()
                  << " | Rate: " << std::fixed << std::setprecision(1)
                  << (context.GetAttempts() * 1000.0 / timer.GetElapsed()) << " pwd/sec"
                  << " | Threads: " << threadCount;
    }

    // Wait for all threads
    for (auto& t : workers) t.join();

    timer.Stop();

    // Display results
    if (context.IsFound()) {
        Console::PrintSuccess("PASSWORD FOUND!");
        std::cout << "Login: " << context.foundLogin << "\n";
        std::cout << "Password: " << context.foundPassword << "\n";
        std::cout << "Attempts: " << context.GetAttempts() << "\n";
        std::cout << "Time: " << timer.GetElapsedFormatted() << "\n";
        std::cout << "Threads: " << threadCount << "\n";
    }
}
```

### 3.2 Modify ModeRuleBasedAttack()
**File**: `client/main.cpp`

Similar updates to ModeBruteForce() but using RuleBasedAttack generator.

## Phase 4: Build System Updates (1 hour)

### 4.1 Update Build Script
**File**: `build.bat`

```batch
@echo off
echo Building with C++11 support...

:: Server (requires Common Controls)
cd server
g++ -std=c++11 -O2 -o test.exe test.cpp -lcomctl32 -lgdi32 -mwindows

:: Client
cd ..\client
g++ -std=c++11 -O2 -o client.exe main.cpp BruteForce.cpp PipeClient.cpp RuleAttack.cpp Utils.cpp

cd ..
echo Build complete!
```

## Phase 5: Testing Strategy (2-3 hours)

### 5.1 Unit Tests

1. **Server Thread Configuration**
   - Start with default threads → verify auto-detection
   - Set to 3 (minimum) → verify minimum enforcement
   - Set to 255 (maximum) → verify cap enforcement

2. **Client Thread Validation**
   - Test thread count validation (rejects > max)
   - Test auto-detection (uses min(hw, 16))
   - Test early termination (password found → all threads stop)

3. **Performance Scaling**
   - Measure speedup: 1, 2, 4, 8, 16 threads
   - Expected: near-linear scaling up to server thread limit

### 5.2 Integration Tests

1. **Concurrent Connections**
   - Server: 4 threads, Client: 8 threads
   - Expected: 4 succeed, 4 wait or fail gracefully

2. **Full 144-Password Coverage Test**
   - 4 passwords × 3 alphabets × 3 lengths × 2 attacks × 2 server modes
   - Verify all can be cracked within reasonable time

## Critical Implementation Details

### Thread Safety Strategy
- **Generator**: Mutex wrapper serializes Next() calls
- **Statistics**: Atomic counters for performance
- **Results**: Mutex for string operations (not atomic-friendly)

### Performance Considerations
- Each thread owns its PipeClient (no sharing)
- Local attempt counters reduce atomic operations
- Check found flag every 10 iterations (not every password)

### Error Handling
- Failed connections cause thread exit (don't crash others)
- Invalid thread counts rejected with clear message
- Generator exhaustion handled gracefully

## Files Summary

### To Modify:
1. `server/ServerConstants.h` - Remove hardcoded thread limit
2. `server/ServerContext.h` - Add thread management
3. `server/test.cpp` - Add spinner control
4. `server/PipeServer.h` - Enhanced logging
5. `client/ClientConstants.h` - Add thread constants
6. `client/main.cpp` - Rewrite attack functions

### To Create:
1. `client/ThreadSafePasswordGenerator.h` - Thread-safe wrapper
2. `client/AttackContext.h` - Shared state
3. `client/AttackWorker.h` - Worker thread logic

## Time Estimates

| Phase | Description | Time |
|-------|-------------|------|
| 1 | Server thread configuration | 2-3 hrs |
| 2 | Client thread infrastructure | 4-5 hrs |
| 3 | Client attack mode updates | 3-4 hrs |
| 4 | Build system updates | 1 hr |
| 5 | Testing and validation | 2-3 hrs |
| **Total** | **All phases** | **12-16 hrs** |

## Validation for 140% Grade

✓ Server multithreaded with configurable thread count
✓ Client multithreaded with auto-detection and user config
✓ Simple overall stats display (Option 1)
✓ Thread count shown in final summary
✓ Proper synchronization and early termination
✓ OOP principles and clean code practices

## Next Steps

1. Implement Phase 1 (Server Configuration) - 2-3 hours
2. Implement Phase 2 (Client Infrastructure) - 4-5 hours
3. Implement Phase 3 (Attack Modes) - 3-4 hours
4. Test and validate - 2-3 hours

This plan provides a clear path to achieving the 140% grade requirement while maintaining code quality and following best practices.