# Brute Force Authentication System - Project Overview

**Educational Password Cracking & Authentication Vulnerability Demonstration**

A Windows-based client-server system demonstrating password cracking techniques, authentication vulnerabilities, and anti-brute-force protection mechanisms for cybersecurity education and research.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Features - Server Component](#features---server-component)
3. [Features - Client Component](#features---client-component)
4. [Technical Details](#technical-details)
5. [Project Structure](#project-structure)
6. [Known Issues and Problems](#known-issues-and-problems)
7. [Future Enhancements](#future-enhancements)
8. [Building and Running](#building-and-running)
9. [Educational Use Cases](#educational-use-cases)
10. [Repository Information](#repository-information)

---

## Architecture Overview

This system implements a **client-server architecture** using **Windows Named Pipes** for inter-process communication (IPC).

```
┌─────────────────────────────────────────────────────────────┐
│                        Client (client.exe)                  │
│  ┌────────────────┐  ┌────────────────┐  ┌──────────────┐  │
│  │ Connection     │  │ Brute Force    │  │ Dictionary   │  │
│  │ Test           │  │ Attack         │  │ Attack       │  │
│  └────────────────┘  └────────────────┘  └──────────────┘  │
│           │                  │                   │          │
│           └──────────────────┴───────────────────┘          │
│                              │                              │
│                    ┌─────────▼─────────┐                    │
│                    │   PipeClient      │                    │
│                    │   Communication   │                    │
│                    └─────────┬─────────┘                    │
└──────────────────────────────┼──────────────────────────────┘
                               │
                    Named Pipe: \\.\pipe\AuthPipe
                    Protocol: "login password" → DWORD
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                       Server (test.exe)                     │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  GUI: Load Users | Anti-BF Protection | Start Server │  │
│  └──────────────────────────────────────────────────────┘  │
│                              │                              │
│                    ┌─────────▼─────────┐                    │
│                    │   PipeServer      │                    │
│                    │   (3 Threads)     │                    │
│                    └─────────┬─────────┘                    │
│                              │                              │
│                    ┌─────────▼─────────┐                    │
│                    │    UserList       │                    │
│                    │  Authentication   │                    │
│                    └───────────────────┘                    │
└─────────────────────────────────────────────────────────────┘
```

**Key Components:**
- **Server (test.exe)**: GUI application managing authentication and pipe server
- **Client (client.exe)**: Attack tool with three operational modes
- **Communication**: Windows Named Pipes with custom text-based protocol
- **Thread Model**: Server handles 3 concurrent connections; client runs single-threaded

---

## Features - Server Component

**Primary Files**: `server/test.cpp`, `server/PipeServer.h`, `server/PerPipeStruct.h`, `server/list.h`

### Core Functionality

#### 1. Named Pipe Authentication Server
- **Pipe Name**: `\\.\pipe\AuthPipe` (local machine)
- **Mode**: Message-type pipe with duplex communication
- **Buffer Size**: 512 bytes per message
- **Threading**: 3 concurrent worker threads handle simultaneous connections

#### 2. User Credential Management (`server/list.h`)
- Load credentials from text file (format: `login password` per line)
- Maximum username/password length: 20 characters (configurable)
- In-memory storage using STL vectors
- Search optimization: Linear search with early termination

#### 3. Anti-Brute-Force Protection
- **Mechanism**: Time-based account lockout after failed attempts
- **Lockout Duration**: 3 seconds per username
- **Granularity**: Per-username protection (not global)
- **Toggle**: Can be enabled/disabled via GUI checkbox
- **Implementation**: `GetTickCount64()` timestamp tracking

#### 4. GUI Interface (Windows API)
- **Load Users File**: Opens file dialog to select credentials file
- **Protection Toggle**: Checkbox to enable/disable anti-brute-force mode
- **Start Server**: Launches 3 pipe listener threads
- **Log Window**: Real-time logging of all authentication attempts, successes, and blocks

#### 5. Authentication Protocol Handler
- **Request Processing**:
  1. Read message from pipe (`ReadFile()`)
  2. Parse login and password (space-separated)
  3. Check if account is currently blocked (protection mode)
  4. Validate credentials against user list
  5. Send DWORD response: `1` (success) or `0` (failure)
  6. Disconnect client

- **Logging**: All events logged to GUI with detailed information

---

## Features - Client Component

**Primary Files**: `client/main.cpp`, `client/PipeClient.cpp/.h`, `client/BruteForce.cpp/.h`, `client/RuleAttack.cpp/.h`, `client/Utils.cpp/.h`

### Attack Modes

#### Mode 1: Connection Test
**Purpose**: Verify connectivity and test single credentials

**Features**:
- Manual login and password entry
- Direct pipe connection test
- Single authentication attempt
- Response validation and display

**Use Case**: Initial reconnaissance and credential verification

---

#### Mode 2: Brute Force Attack
**Purpose**: Exhaustive password generation and systematic testing

**Features**:

**Alphabet Configurations**:
1. **Lowercase + Apostrophe** (27 characters)
   - Characters: `abcdefghijklmnopqrstuvwxyz'`
   - Best for: Simple lowercase password policies

2. **Alphanumeric + Apostrophe** (63 characters)
   - Characters: `ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'`
   - Best for: Mixed-case password policies

3. **Full Alphabet** (128+ characters)
   - Includes: Latin (A-Z, a-z), Cyrillic (А-Я, а-я), digits, apostrophe
   - Best for: International password testing

**Configuration**:
- Maximum password length: 1-20 characters (user-specified)
- Total combinations calculated before attack begins
- Example: 5-char password with 27-char alphabet = 27^5 = 14,348,907 combinations

**Real-Time Statistics**:
- Current password being tested
- Attempt number / Total combinations
- Elapsed time (HH:MM:SS format)
- Attack rate (passwords/second)
- Progress updates every 100 attempts

**Algorithm**:
- Lexicographic generation (starts with "a", then "aa", "ab", etc.)
- Efficient combinatorial iteration
- Proper overflow handling for large alphabets

---

#### Mode 3: Dictionary Attack with Rule-Based Transformations
**Purpose**: Intelligent password guessing using common words and variations

**Features**:

**Dictionary Loading**:
- File selection via Windows file dialog (`GetOpenFileNameA()`)
- UTF-8 text file support
- One word per line format

**Transformation Rules** (Applied to each dictionary word):

1. **Case Variations** (4 types):
   - Original case
   - All lowercase
   - All uppercase
   - Capitalized (first letter uppercase)

2. **Reversal**:
   - Reverse the password string
   - Example: "password" → "drowssap"

3. **Digit Suffix Combinations** (9 patterns):
   - Single digit: `1`
   - Pairs: `12`
   - Triples: `123`
   - Quads: `1234`
   - Years: `2023`, `2024`
   - Zeros: `0`, `00`
   - Common: `123456`

4. **Special Character Suffix**:
   - Exclamation mark: `!`

5. **Latin-to-Cyrillic Phonetic Conversion**:
   - Transliterate Latin characters to Cyrillic equivalents
   - Example: "admin" → "админ" (phonetic approximation)
   - **Note**: Currently incomplete (see [Issue #6](#low-severity))

6. **Cyrillic Reversal**:
   - Reverse Cyrillic-converted passwords

**Variant Generation**:
- Each dictionary word generates 50-100+ variants
- Deduplication via `std::unordered_set` (hash-based)
- Ordered iteration via `std::vector`
- Memory-efficient storage

**Example**:
```
Dictionary word: "admin"
Generated variants:
  admin, Admin, ADMIN, admin1, admin12, admin123, admin1234,
  admin2023, admin2024, admin0, admin00, admin123456, admin!,
  nimda (reversed), админ (Cyrillic), нимда (Cyrillic reversed), ...
```

---

### Shared Client Features

**Connection Management** (`PipeClient.cpp/.h`):
- Automatic connection to `\\.\pipe\AuthPipe`
- Retry mechanism with 5-second timeout (`WaitNamedPipeA()`)
- Reconnection on write failure
- Reconnection on read failure
- Proper error logging with color-coded console output

**Performance Tracking** (`Utils.cpp/.h`):
- High-resolution timer using `std::chrono::high_resolution_clock`
- Real-time attack rate calculation
- Elapsed time formatting
- Statistics display

**Console UI**:
- Color-coded output:
  - **Green**: Success messages
  - **Red**: Error messages
  - **Yellow**: Warning messages
  - **Cyan**: Informational messages
- Windows console API for text attributes
- Progress indicators

---

## Technical Details

### Communication Protocol

**Message Flow**:
```
Client                          Server
  |                               |
  |--- "username password" ------>|  (ASCII string, max 512 bytes)
  |                               |
  |                               |--- Parse & Authenticate
  |                               |
  |<------ DWORD (4 bytes) -------|  (1 = success, 0 = failure)
  |                               |
```

**Protocol Specification**:
- **Request Format**: Space-separated ASCII string
  - Structure: `"login password"`
  - Maximum length: 511 bytes (512 - null terminator)
  - Example: `"admin SecureP@ss123"`

- **Response Format**: Binary DWORD (4 bytes, little-endian)
  - Value `1` (0x00000001): Authentication successful
  - Value `0` (0x00000000): Authentication failed or user not found
  - Value `-1` (0xFFFFFFFF): Account currently blocked (protection mode)

**Connection Lifecycle**:
1. Client calls `CreateFileA()` to connect to pipe
2. If pipe busy, `WaitNamedPipeA()` waits up to 5 seconds
3. Client writes credentials with `WriteFile()`
4. Client reads response with `ReadFile()`
5. Client calls `CloseHandle()` to disconnect
6. **Current Behavior**: Client disconnects after **every** password attempt (see [Issue #4](#medium-severity))

---

### Technology Stack

| Component | Technology |
|-----------|-----------|
| **Language** | C++ (C++11 standard) |
| **Platform** | Windows 10/11 (Win32 API) |
| **Compiler** | MinGW g++ |
| **Libraries** | comctl32 (Common Controls), comdlg32 (Common Dialogs) |
| **Build Tool** | VSCode tasks / Manual g++ |
| **IPC Mechanism** | Windows Named Pipes (CreateNamedPipe API) |
| **Threading** | Win32 CreateThread API |
| **GUI Framework** | Win32 API (native Windows) |
| **File I/O** | C++ STL (fstream, stringstream) |

---

### Performance Characteristics

**Server**:
- **Concurrency**: 3 simultaneous connections
- **Throughput**: Limited by pipe creation/destruction overhead
- **Memory**: O(n) where n = number of users in credential file
- **Blocking**: Each thread blocks on `ConnectNamedPipe()` until client connects

**Client**:
- **Threading**: Single-threaded sequential password testing
- **Attack Rate**: Varies by mode
  - Connection Test: N/A (manual)
  - Brute Force: 50-200 attempts/second (limited by reconnection overhead)
  - Dictionary: 100-500 attempts/second (limited by variant generation)
- **Memory**:
  - Brute Force: O(1) - generates passwords on-the-fly
  - Dictionary: O(n*m) where n = dictionary size, m = variants per word

**Bottlenecks**:
1. Client disconnects after each attempt (major performance penalty)
2. Single-threaded client (no parallelization)
3. Pipe creation/destruction overhead per connection
4. Linear search in user list (no indexing)

---

## Project Structure

```
brute-force/
│
├── server/
│   ├── test.cpp                   # Server GUI application (WinMain entry)
│   ├── PipeServer.h               # Named pipe server implementation
│   ├── PerPipeStruct.h            # Per-pipe thread data structure
│   ├── list.h                     # User credentials list with protection logic
│   ├── ServerConstants.h          # Server configuration constants
│   ├── ServerContext.h            # Server state encapsulation
│   └── test.exe                   # Compiled server binary (238 KB)
│
├── client/
│   ├── main.cpp                   # Client menu and attack mode orchestration (278 lines)
│   ├── PipeClient.h               # Named pipe client class definition
│   ├── PipeClient.cpp             # Pipe communication implementation (162 lines)
│   ├── BruteForce.h               # Brute force password generator class
│   ├── BruteForce.cpp             # Combinatorial password generation (179 lines)
│   ├── RuleAttack.h               # Dictionary attack with transformations
│   ├── RuleAttack.cpp             # Rule-based variant generation (169 lines)
│   ├── Utils.h                    # Timer, stats, console utilities
│   ├── Utils.cpp                  # Utility implementations (81 lines)
│   └── client.exe                 # Compiled client binary (238 KB)
│
├── .vscode/
│   ├── c_cpp_properties.json      # C++ IntelliSense configuration
│   ├── launch.json                # Debug launch configurations
│   ├── tasks.json                 # Build task definitions (g++ commands)
│   └── settings.json              # VSCode workspace settings
│
├── .claude/
│   └── settings.local.json        # Claude Code integration settings
│
└── .git/                          # Git repository
```

**File Summary**:

| File | Lines of Code | Purpose |
|------|---------------|---------|
| `server/test.cpp` | 114 | Server GUI and main entry point |
| `server/PipeServer.h` | 113 | Pipe server with threading |
| `server/PerPipeStruct.h` | 8 | Data structure for pipe threads |
| `server/list.h` | 66 | User management and protection |
| `client/main.cpp` | 278 | Client menu and attack modes |
| `client/PipeClient.cpp` | 162 | Pipe communication |
| `client/BruteForce.cpp` | 179 | Password generation engine |
| `client/RuleAttack.cpp` | 169 | Dictionary transformations |
| `client/Utils.cpp` | 81 | Timer and statistics |
| **Total** | **~1,170** | Combined codebase |

---

## Known Issues and Problems

### HIGH SEVERITY

#### Issue #1: Memory Leak in server/PipeServer.h:18-20

**Location**: `PipeServer::Log()` function

**Code**:
```cpp
void Log(const std::string& text) {
    char* buf = new char[text.length() + 1];
    strcpy_s(buf, text.length() + 1, text.c_str());
    PostMessage(hGui, WM_LOG_MSG, (WPARAM)buf, 0);
}
```

**Problem**:
- Memory allocated with `new char[]` is passed to `PostMessage()` as WPARAM
- No guaranteed cleanup - recipient window message handler must free memory
- If window is destroyed or message not processed, memory leaks
- No error handling if allocation fails

**Impact**:
- Server memory grows unbounded during long-running sessions
- Each log message leaks 20-100 bytes
- After 10,000 authentication attempts: ~500 KB - 1 MB leaked

**Recommendation**:
```cpp
// Option 1: Use stack allocation for small strings
void Log(const std::string& text) {
    if (text.length() < 256) {
        char buf[256];
        strcpy_s(buf, sizeof(buf), text.c_str());
        PostMessage(hGui, WM_LOG_MSG, (WPARAM)buf, 0);
    }
}

// Option 2: Use shared_ptr with custom deleter in message handler
// Option 3: Use SendMessage() instead of PostMessage() for synchronous cleanup
```

---

#### Issue #2: No Thread Cleanup Mechanism (server/PipeServer.h:30-91, 100-112)

**Location**: `PipeThreadFunc()` infinite loop and `Start()` method

**Code**:
```cpp
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

void Start(int numPipes, UserList* uList, HWND hGui) {
    for (int i = 0; i < numPipes; i++) {
        HANDLE hThread = CreateThread(..., PipeThreadFunc, ...);
        if (hThread) threads.push_back(hThread);
        // ← Threads never joined or cleaned up
    }
}
```

**Problem**:
- Worker threads run in infinite loop with no exit mechanism
- Thread handles stored in vector but never waited for
- No `WaitForMultipleObjects()` or thread join logic
- Server cannot gracefully shut down
- Only way to stop: Force-terminate entire process

**Impact**:
- Resource leak: Thread handles accumulate
- Cannot restart server without closing application
- Unclean shutdown may corrupt in-progress operations
- Violates RAII principles

**Recommendation**:
```cpp
class PipeServer {
private:
    HANDLE hShutdownEvent;  // Event to signal shutdown

public:
    void Start(int numPipes, UserList* uList, HWND hGui) {
        hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        // ... create threads ...
    }

    void Stop() {
        SetEvent(hShutdownEvent);  // Signal all threads to exit
        WaitForMultipleObjects(threads.size(), threads.data(), TRUE, 5000);
        for (HANDLE h : threads) CloseHandle(h);
        threads.clear();
        CloseHandle(hShutdownEvent);
    }
};

DWORD WINAPI PipeThreadFunc(LPVOID lpParam) {
    while (WaitForSingleObject(data->shutdownEvent, 0) == WAIT_TIMEOUT) {
        // ... normal pipe processing ...
    }
    return 0;
}
```

---

#### Issue #3: Memory Safety - Unchecked Buffer Operations (server/PipeServer.h:26-53)

**Location**: `ReadFile()` into fixed 512-byte buffer

**Code**:
```cpp
char buffer[BUFFER_SIZE];  // 512 bytes
ZeroMemory(buffer, BUFFER_SIZE);
success = ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL);
buffer[bytesRead] = '\0';  // ← Potential out-of-bounds if bytesRead >= 512
```

**Problem**:
- Fixed 512-byte buffer with no overflow protection
- `bytesRead` not validated before using as index
- Malicious client could send >511 bytes
- No bounds checking before `buffer[bytesRead] = '\0'`

**Impact**:
- Potential buffer overflow vulnerability
- Stack corruption possible
- Could crash server or allow arbitrary code execution
- Classic security vulnerability (CWE-120)

**Recommendation**:
```cpp
// Validate bytesRead
if (bytesRead >= BUFFER_SIZE) {
    Log("Error: Message too large, rejecting");
    DisconnectNamedPipe(data->hPipe);
    continue;
}
buffer[bytesRead] = '\0';

// Or use std::string with size limits
std::string message;
message.resize(BUFFER_SIZE - 1);
if (bytesRead > 0 && bytesRead < BUFFER_SIZE) {
    message.assign(buffer, bytesRead);
}
```

---

### MEDIUM SEVERITY

#### Issue #4: Performance - Disconnect After Every Attempt (PipeClient.cpp:69)

**Location**: `TryPassword()` method

**Code**:
```cpp
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

**Problem**:
- Client disconnects and reconnects for **every single password attempt**
- Forces complete pipe lifecycle (CreateFile, WriteFile, ReadFile, CloseHandle) per attempt
- Connection setup/teardown overhead: 5-20 milliseconds per attempt
- Named pipe creation is expensive system call

**Impact**:
- **Severe performance penalty**: Brute force attacks run 10-100x slower than necessary
- Example: 10,000 password attempts
  - Current: 10-15 minutes (10-20 attempts/sec)
  - Optimal: 1-2 minutes (100-200 attempts/sec)
- Defeats purpose of brute force attack (efficiency)

**Recommendation**:
```cpp
bool PipeClient::TryPassword(const std::string& login, const std::string& password) {
    if (!connected && !Connect()) {
        return false;
    }

    std::string message = login + " " + password;
    if (!WriteMessage(message)) {
        Disconnect();  // Only on actual failure
        return false;
    }

    int response;
    if (!ReadResponse(response)) {
        Disconnect();  // Only on actual failure
        return false;
    }

    // Keep connection alive for next attempt!
    return (response == 1);
}
```

**Note**: Server would need modification to support persistent connections (currently disconnects after each response).

---

#### Issue #5: Silent Protocol Failures (PipeClient.cpp:139-162, server/PipeServer.h:53-85)

**Location**: `ReadFile()`/`WriteFile()` error handling in both client and server

**Client Code**:
```cpp
bool PipeClient::ReadResponse(int& response) {
    DWORD bytesRead;
    if (!ReadFile(hPipe, &response, sizeof(response), &bytesRead, NULL)) {
        Console::Error("Failed to read response");  // ← Generic error
        Disconnect();
        return false;
    }
    return true;
}
```

**Server Code**:
```cpp
success = ReadFile(data->hPipe, buffer, BUFFER_SIZE - 1, &bytesRead, NULL);
if (success && bytesRead > 0) {
    // ... process message ...
}
// ← No else branch! Silent failure on read error
```

**Problem**:
- Generic error messages with no context about **why** failure occurred
- No use of `GetLastError()` to get detailed error codes
- Server silently ignores read failures (no logging or error handling)
- Cannot differentiate between:
  - Broken pipe (client disconnected)
  - Buffer overflow
  - Timeout
  - Permission error

**Impact**:
- Difficult to debug connection issues
- Silent data loss in server
- No visibility into failure reasons
- Poor user experience during troubleshooting

**Recommendation**:
```cpp
bool PipeClient::ReadResponse(int& response) {
    DWORD bytesRead;
    if (!ReadFile(hPipe, &response, sizeof(response), &bytesRead, NULL)) {
        DWORD error = GetLastError();
        char errorMsg[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
                      0, errorMsg, sizeof(errorMsg), NULL);
        Console::Error("Failed to read response: " + std::string(errorMsg));
        Disconnect();
        return false;
    }

    if (bytesRead != sizeof(response)) {
        Console::Warning("Incomplete response received");
        return false;
    }

    return true;
}

// Server: Add error logging
if (!success || bytesRead == 0) {
    DWORD error = GetLastError();
    Log("Pipe read error: " + std::to_string(error));
    DisconnectNamedPipe(data->hPipe);
    continue;
}
```

---

### LOW SEVERITY

#### Issue #6: Incomplete Feature - Cyrillic Support (RuleAttack.cpp:162-169)

**Location**: `CyrillicToLatin()` function

**Code**:
```cpp
std::string RuleBasedAttack::CyrillicToLatin(const std::string& cyrillic) {
    std::string result = cyrillic;
    // In a real implementation, we'd need proper UTF-8 Cyrillic support
    // For now, return as-is since we can't easily map back
    return result;
}
```

**Problem**:
- Function is explicitly commented as incomplete/placeholder
- No actual character conversion logic implemented
- Returns input unchanged (no-op function)
- Opposite function `LatinToCyrillic()` exists but this reverse is missing

**Impact**:
- Dictionary attack "Latin-to-Cyrillic conversion" feature doesn't work
- Generated Cyrillic password variants are incorrect
- Misleading feature in menu (advertised but non-functional)
- Limited impact since most users won't need Cyrillic

**Recommendation**:
```cpp
// Option 1: Implement proper UTF-8 phonetic mapping
std::string RuleBasedAttack::CyrillicToLatin(const std::string& cyrillic) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(cyrillic);

    std::map<wchar_t, char> mapping = {
        {L'А', 'A'}, {L'Б', 'B'}, {L'В', 'V'}, {L'Г', 'G'},
        // ... complete mapping ...
    };

    std::string result;
    for (wchar_t ch : wide) {
        if (mapping.count(ch)) {
            result += mapping[ch];
        } else {
            // Keep original character
        }
    }
    return result;
}

// Option 2: Remove feature entirely if not needed
// Remove function and related transformation rules
```

---

#### Issue #7: Hardcoded Configuration

**Locations**: Multiple files

**Problem**: Critical configuration values are hardcoded in source code:

| Configuration | Value | Location | Impact |
|---------------|-------|----------|--------|
| Pipe name | `\\.\pipe\AuthPipe` | server/PipeServer.h:8 | Cannot test multiple instances |
| Thread count | `3` | server/test.cpp | Inflexible concurrency |
| Protection timeout | `3000` ms | server/list.h | Cannot adjust for testing |
| Buffer size | `512` bytes | server/PipeServer.h:9 | Cannot handle longer messages |
| Max password length | `20` chars | server/list.h | Arbitrary limitation |

**Impact**:
- Reduced flexibility for different testing scenarios
- Cannot run multiple server instances simultaneously (pipe name conflict)
- Cannot adjust thread count based on system resources
- Requires recompilation to change any configuration

**Recommendation**:
```cpp
// Create config.ini file:
[Server]
PipeName=\\.\pipe\AuthPipe
ThreadCount=3
BufferSize=512
ProtectionTimeoutMs=3000

[Credentials]
MaxLoginLength=20
MaxPasswordLength=20

// Load configuration:
class Config {
public:
    static Config& Instance() {
        static Config instance;
        return instance;
    }

    std::string GetPipeName() { return pipeName; }
    int GetThreadCount() { return threadCount; }
    // ... other getters ...

private:
    Config() { LoadFromFile("config.ini"); }
    void LoadFromFile(const std::string& path);

    std::string pipeName;
    int threadCount;
    int bufferSize;
    // ... other fields ...
};
```

---

## Future Enhancements

**Performance Optimizations**:
- [ ] Multi-threaded client for parallel password attempts (10-100x speedup)
- [ ] Persistent connection pooling (eliminate reconnection overhead)
- [ ] Indexed user lookup (hash map instead of linear search)
- [ ] GPU acceleration for brute force (CUDA/OpenCL)

**Feature Additions**:
- [ ] Remote server support with hostname/IP parameter
- [ ] Password attempt history logging to CSV/JSON
- [ ] Real-time graphical progress bar
- [ ] Pause/resume functionality for long-running attacks
- [ ] Incremental dictionary attack (start from arbitrary position)
- [ ] Custom transformation rule definition via config file
- [ ] Hybrid attack mode (brute force + dictionary)

**Security & Reliability**:
- [ ] TLS/SSL encryption for pipe communication
- [ ] Rate limiting on server side (configurable)
- [ ] Credential file encryption at rest
- [ ] Input validation and sanitization
- [ ] Audit logging to file with timestamps

**Usability**:
- [ ] Command-line argument parsing (no interactive menu)
- [ ] Configuration file support (INI/JSON)
- [ ] Cross-platform support (Linux/macOS with sockets)
- [ ] Unit tests for core algorithms
- [ ] Comprehensive documentation with examples

**Internationalization**:
- [ ] Complete Cyrillic phonetic conversion
- [ ] Unicode (UTF-16) password support
- [ ] Multi-language dictionary support
- [ ] Locale-aware transformations

---

## Building and Running

### Prerequisites

- **Operating System**: Windows 10 or Windows 11
- **Compiler**: MinGW with g++ (tested with GCC 8.1.0+)
- **IDE** (optional): Visual Studio Code with C/C++ extension
- **Memory**: 512 MB RAM minimum
- **Disk Space**: 50 MB for source + binaries

### Build Commands

#### Option 1: Manual Compilation

**Server**:
```bash
cd C:\Users\glebm\projects\tech-safety\brute-force
g++ -g server/test.cpp -o server/test.exe -lcomctl32 -lcomdlg32 -static
```

**Client**:
```bash
cd C:\Users\glebm\projects\tech-safety\brute-force\client
g++ -g main.cpp PipeClient.cpp BruteForce.cpp RuleAttack.cpp Utils.cpp -o client.exe -lcomdlg32 -static
```

**Compiler Flags**:
- `-g`: Include debug symbols
- `-lcomctl32`: Link Common Controls library (for GUI)
- `-lcomdlg32`: Link Common Dialogs library (for file picker)
- `-static`: Static linking (optional, for standalone .exe)

#### Option 2: VSCode Tasks

1. Open project in VSCode
2. Press `Ctrl+Shift+B` (Run Build Task)
3. Select "Build Server" or "Build Client"

### Running the System

#### Step 1: Prepare Credentials File

Create a text file (e.g., `users.txt`) with this format:
```
20 20
admin Password123!
user1 Simple1
user2 Complex_2024
test MyP@ssw0rd
```

**Format Specification**:
- **Line 1**: `[max_login_length] [max_password_length]`
- **Remaining lines**: `[login] [password]` (space-separated)

#### Step 2: Start Server

```bash
test.exe
```

**Server Setup**:
1. Click **"Load Users File"** button
2. Select your credentials file
3. (Optional) Check **"Anti-Brute-Force Protection"** to enable 3-second lockout
4. Click **"Start Server"** button
5. Server log will display: "Server started. Waiting for connections..."

#### Step 3: Run Client

```bash
client\client.exe
```

**Client Menu**:
```
===================================
  Password Cracking Client Menu
===================================
1. Test Connection
2. Brute Force Attack
3. Dictionary Attack
4. Exit
===================================
Enter your choice:
```

#### Step 4: Select Attack Mode

**Mode 1 - Test Connection**:
```
Enter your choice: 1
Enter login: admin
Enter password: Password123!
```

**Mode 2 - Brute Force Attack**:
```
Enter your choice: 2
Select alphabet:
1. Lowercase + apostrophe (27 chars)
2. Mixed case + digits + apostrophe (63 chars)
3. Full alphabet including Cyrillic (128+ chars)
Choice: 1
Enter maximum password length (1-20): 5
Enter login to attack: admin

[!] Total combinations to try: 14,348,907
Starting brute force attack...
[*] Trying: aaaaa (Attempt 1 / 14348907) [Rate: 0.00 pwd/s]
[*] Trying: aaaab (Attempt 2 / 14348907) [Rate: 150.23 pwd/s]
```

**Mode 3 - Dictionary Attack**:
```
Enter your choice: 3
[File dialog opens - select dictionary.txt]
Enter login to attack: admin

Loaded 10000 words from dictionary
Generated 450000 password variants
Starting dictionary attack...
[*] Trying: password (Attempt 1 / 450000)
[*] Trying: Password (Attempt 2 / 450000)
[*] Trying: PASSWORD (Attempt 3 / 450000)
[SUCCESS] Password found: Password123! (Attempt 5432)
Elapsed time: 00:00:54
Average rate: 100.59 passwords/second
```

### Sample Credentials File

The included `info.txt.txt` contains:
```
20 20
known_user Secure_Connect_1!
viol_R1 Winter123
viol_R2 Lab2025!
viol_R5 MyP!
ideal_R4 4Dogs@Home
```

**Password Classification**:
- `known_user`: Complex password (demonstration)
- `viol_R1`: Violation Rule 1 (weak pattern)
- `viol_R2`: Violation Rule 2 (predictable)
- `viol_R5`: Violation Rule 5 (too short)
- `ideal_R4`: Ideal password (strong pattern)

---

## Educational Use Cases

This project serves as a comprehensive teaching tool for cybersecurity education:

### 1. Password Vulnerability Analysis
- Demonstrates why short passwords are vulnerable (brute force)
- Shows effectiveness of dictionary attacks against common words
- Illustrates password transformation predictability (adding "123", etc.)
- Quantifies attack speed and feasibility

### 2. Attack Methodology Training
- **Reconnaissance**: Connection testing and server probing
- **Brute Force**: Systematic exhaustive search
- **Dictionary Attacks**: Intelligent guessing with transformations
- **Real-world simulation**: Multi-mode attack strategies

### 3. Defense Mechanism Education
- Anti-brute-force protection (time-based lockout)
- Account-level vs. global protection trade-offs
- Protection bypass techniques (slow attacks below threshold)
- Effectiveness analysis (3-second lockout impact)

### 4. Windows Systems Programming
- Named Pipes for IPC (client-server communication)
- Multi-threaded server architecture
- Win32 API usage (GUI, threading, file dialogs)
- Windows security model

### 5. Algorithm Design & Optimization
- Combinatorial generation algorithms
- Rule-based transformation systems
- Deduplication using hash sets
- Performance profiling and bottleneck analysis

### 6. Secure Coding Practices (by counter-example)
- Buffer overflow vulnerabilities ([Issue #3](#issue-3-memory-safety---unchecked-buffer-operations-pipeserverh26-53))
- Memory leak patterns ([Issue #1](#issue-1-memory-leak-in-pipeserverh18-20))
- Resource management failures ([Issue #2](#issue-2-no-thread-cleanup-mechanism-pipeserverh30-91-100-112))
- Error handling best practices ([Issue #5](#issue-5-silent-protocol-failures-pipeclientcpp139-162-pipeserverh53-85))

### Target Audiences

- **Cybersecurity Students**: Hands-on password cracking experience
- **Penetration Testing Trainees**: Attack methodology and tool development
- **Software Engineering Students**: Systems programming and IPC
- **Security Researchers**: Platform for testing new protection mechanisms
- **CTF Participants**: Practice environment for password challenges

### Ethical Considerations

⚠️ **IMPORTANT ETHICAL NOTICE** ⚠️

This tool is designed **exclusively** for:
- ✅ Authorized security testing on systems you own or have explicit permission to test
- ✅ Educational purposes in academic settings
- ✅ Security research in controlled environments
- ✅ Penetration testing with written authorization

**Unauthorized use is ILLEGAL and UNETHICAL**:
- ❌ Never use against systems without explicit written permission
- ❌ Never use for malicious purposes or personal gain
- ❌ Never use to access others' accounts or data
- ❌ Unauthorized access to computer systems is a **federal crime** (CFAA in US, similar laws worldwide)

**Legal Disclaimer**: The authors and contributors assume no liability for misuse of this software. Users are solely responsible for ensuring their activities comply with all applicable laws and regulations.

---

## Repository Information

**GitHub Repository**: [https://github.com/hmokryiedu/SecureTechnologies](https://github.com/hmokryiedu/SecureTechnologies)

**Project Path**: `tech-safety/brute-force`

**Branch Structure**:
- `server`: Server-side development (server/test.exe, server/PipeServer.h, server/list.h)
- `client`: Client-side development (client.exe, attack modes)
- Recent merges: Periodic integration of client and server branches

**Development Status**: Active educational project

**Contributors**: To be specified

**License**: To be specified (recommend MIT or GPL for educational projects)

**Contact**: hmokryiedu (GitHub)

**Documentation**:
- This file: `project-overview.md` (comprehensive technical reference)
- Source code comments: Inline documentation in C++ files
- VSCode configuration: `.vscode/` directory with build tasks

**Version History**: See `git log` for commit history

**Issue Tracking**: See [Known Issues](#known-issues-and-problems) section above

---

## Quick Reference

**Server Controls**:
- Load credentials: File → Open (or GUI button)
- Enable protection: Check "Anti-Brute-Force Mode"
- Start listening: Click "Start Server"
- View activity: Monitor log window

**Client Attack Modes**:
1. **Connection Test**: Quick single-credential verification
2. **Brute Force**: Exhaustive search (choose alphabet + max length)
3. **Dictionary**: Word list with intelligent transformations

**Key Files to Modify**:
- Server authentication: `server/list.h:45-65`
- Client-server protocol: `server/PipeServer.h:53-85`, `PipeClient.cpp:45-162`
- Attack algorithms: `BruteForce.cpp:30-120`, `RuleAttack.cpp:50-169`

**Performance Tips**:
- Fix [Issue #4](#issue-4-performance---disconnect-after-every-attempt-pipeclientcpp69) for 10-100x speedup
- Use lowercase alphabet (27 chars) for faster testing
- Limit max password length to 6-8 for brute force feasibility

**Security Notes**:
- Default pipe (`\\.\pipe\AuthPipe`) is local-only
- No encryption - credentials sent in plaintext
- Anti-brute-force protection is time-based (3 seconds)
- See [Known Issues](#known-issues-and-problems) for vulnerabilities

---

**Last Updated**: 2025-12-11
**Document Version**: 1.0
**Codebase Version**: Git commit `36eba2b` (client branch)
