# Server Multithreading & Race Condition Fix

## Current Status Analysis

### ✅ Multithreading: IMPLEMENTED
- Server creates **3 worker threads** using `CreateThread()` (PipeServer.h:102-110)
- Each thread runs `PipeInstanceThread()` to handle clients independently
- **Meets requirements.md:29** - "Create at least 3 instances of a Named Pipe" ✓

### ❌ Async/Overlapped I/O: NOT IMPLEMENTED (Optional)
- `CreateNamedPipe()` uses `PIPE_ACCESS_DUPLEX` without `FILE_FLAG_OVERLAPPED`
- Requirements.md:29 states async is "**recommended**" (not mandatory)
- Current synchronous approach works correctly
- **Decision**: Not implementing async I/O (focus on critical bug fix)

### 🔴 CRITICAL BUG: Race Condition on blockedUsers Map
- **Location**: `list.h:20` - `std::map<std::string, DWORD> blockedUsers`
- **Problem**: Shared across 3 threads without synchronization
- **Impact**:
  - Map corruption under concurrent access
  - Anti-cracking protection fails (100% grade requirement)
  - Crashes from iterator invalidation
  - Violates requirements.md:39-41 (protection must not freeze server)

---

## Problem: Race Condition in Anti-Cracking Protection

### What is the Issue?
The `UserList` class has a `blockedUsers` map that tracks which logins are temporarily blocked after failed authentication attempts. This map is accessed by **3 concurrent threads** without any synchronization, causing:

1. **Data corruption**: Multiple threads modifying the map simultaneously
2. **Crashes**: Iterator invalidation when one thread erases while another reads
3. **Incorrect behavior**: Blocks may not work correctly, defeating the anti-cracking protection
4. **Requirements violation**: Fails requirements.md:39-41 (delay must not freeze server)

### Where Does It Happen?

**File:** `list.h:75-105` - `CheckUser()` method

**Unprotected Operations:**
1. **Lines 77-85**: Check if user is blocked (READ from map)
2. **Line 92**: Clear block on successful password (WRITE to map)
3. **Line 98**: Add block on failed password (WRITE to map)

### Why Is This a Problem?

```cpp
// Thread 1 (Client A attacking "admin"):
if (blockedUsers.count(login)) {  // Reading map
    // Thread 2 can modify map here!
}

// Thread 2 (Client B attacking "admin"):
blockedUsers[login] = GetTickCount() + 3000;  // Writing map
// Map corruption! Undefined behavior!
```

---

## Solution: Add CRITICAL_SECTION Synchronization

### Strategy
Use a single `CRITICAL_SECTION` to protect all access to the `blockedUsers` map.

**Why CRITICAL_SECTION?**
- Fast (optimized for same-process threads)
- Simple (single lock = no deadlocks possible)
- Windows native (matches rest of codebase)
- Low contention (only 3 threads)

**Why NOT other options?**
- ❌ `std::mutex`: Inconsistent with Win32 API style
- ❌ `SRWLock`: More complex, introduces TOCTOU issues
- ❌ Per-login locks: Overkill for 3-thread scenario
- ❌ Lock-free: Extremely complex, unjustified

---

## Implementation Plan

### File: `list.h`

#### Change 1: Add Lock Member Variable (after line 23)

```cpp
class UserList {
private:
    struct User {
        string login;
        string password;
    };

    vector<User> users;
    map<string, DWORD> blockedUsers; // Line 20
    bool protectionMode;             // Line 21
    int maxLoginLen;                 // Line 22
    int maxPassLen;                  // Line 23

    // ADD THIS:
    CRITICAL_SECTION blockedUsersLock; // Protects blockedUsers map
```

#### Change 2: Initialize Lock in Constructor (line 26)

```cpp
// BEFORE:
UserList() : protectionMode(false), maxLoginLen(0), maxPassLen(0) {}

// AFTER:
UserList() : protectionMode(false), maxLoginLen(0), maxPassLen(0) {
    InitializeCriticalSection(&blockedUsersLock);
}
```

**Explanation:** `InitializeCriticalSection()` must be called before the lock is used.

#### Change 3: Add Destructor (after line 26)

```cpp
~UserList() {
    DeleteCriticalSection(&blockedUsersLock);
}
```

**Explanation:** Cleanup system resources when `UserList` is destroyed.

#### Change 4: Protect CheckUser() Method (lines 75-105)

**BEFORE (Unsafe):**
```cpp
int CheckUser(const string& login, const string& pass) {
    // Перевірка блокування (Anti-Brute-Force)
    if (protectionMode) {
        DWORD currentTime = GetTickCount();
        if (blockedUsers.count(login)) {  // ⚠️ UNSAFE READ
            if (currentTime < blockedUsers[login]) {
                return -1; // Ігноруємо запит
            } else {
                blockedUsers.erase(login); // ⚠️ UNSAFE WRITE
            }
        }
    }

    // Пошук користувача
    for (const auto& u : users) {
        if (u.login == login) {
            if (u.password == pass) {
                if (protectionMode) blockedUsers.erase(login); // ⚠️ UNSAFE WRITE
                return 1; // Успіх
            } else {
                // Невірний пароль
                if (protectionMode) {
                    blockedUsers[login] = GetTickCount() + 3000; // ⚠️ UNSAFE WRITE
                }
                return 0; // Пароль невірний
            }
        }
    }
    return 0; // Користувача не знайдено
}
```

**AFTER (Thread-Safe):**
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
                // Block expired - remove from map
                blockedUsers.erase(login);
            }
        }

        LeaveCriticalSection(&blockedUsersLock); // 🔓 UNLOCK
    }

    // 2. Search for user and validate password
    // NOTE: This loop does NOT need lock - reads from 'users' vector only
    for (const auto& u : users) {
        if (u.login == login) {
            if (u.password == pass) {
                // Success - clear any existing block
                if (protectionMode) {
                    EnterCriticalSection(&blockedUsersLock); // 🔒 LOCK
                    blockedUsers.erase(login);
                    LeaveCriticalSection(&blockedUsersLock); // 🔓 UNLOCK
                }
                return 1; // Success
            } else {
                // Failed password - add/update block
                if (protectionMode) {
                    EnterCriticalSection(&blockedUsersLock); // 🔒 LOCK
                    blockedUsers[login] = GetTickCount() + 3000; // Block for 3 seconds
                    LeaveCriticalSection(&blockedUsersLock); // 🔓 UNLOCK
                }
                return 0; // Wrong password
            }
        }
    }
    return 0; // User not found
}
```

### Key Design Decisions

1. **3 separate lock acquisitions** (not 1 global lock):
   - First lock: Check if blocked (lines 77-91)
   - Second lock: Clear block on success (lines 95-98)
   - Third lock: Add block on failure (lines 103-106)
   - **Benefit**: Minimizes lock hold time = better concurrency

2. **Users vector NOT locked**:
   - The `users` vector is read-only after initialization
   - `Load()` is called before server starts (no threads yet)
   - No synchronization needed for read-only data

3. **Early return releases lock**:
   - If user is still blocked (line 84), release lock before returning
   - Prevents lock being held unnecessarily

---

## Testing Plan

### Test 1: Single-Client Protection Mode
**Purpose:** Verify basic blocking logic works

```
1. Load users: "admin password123"
2. Enable protection mode
3. Client sends: "admin wrongpass" → Expect: 0 (failure)
4. Client sends: "admin wrongpass" → Expect: -1 (blocked)
5. Wait 3.5 seconds
6. Client sends: "admin password123" → Expect: 1 (success)

✓ Validates: Block works, timeout works, clear on success works
```

### Test 2: Concurrent Attacks on Same Login
**Purpose:** Verify no race condition when multiple threads access same login

```
1. Launch 2 clients simultaneously
2. Both attack "admin" with wrong passwords
3. Verify: No crashes, both see -1 (blocked) or 0 (failure)

✓ Validates: No map corruption, synchronization works correctly
```

### Test 3: Concurrent Attacks on Different Logins
**Purpose:** Verify independent blocking per login

```
1. Launch 3 clients attacking 3 different logins simultaneously
2. All send wrong passwords
3. Verify: Each login blocked independently

✓ Validates: No interference between different logins, server responsive
```

### Test 4: Stress Test (1 hour)
**Purpose:** Verify no deadlocks or stability issues

```
1. Run server continuously with 3 clients
2. Random passwords, mixed logins
3. Toggle protection mode every 5 minutes
4. Monitor for: No hangs, no crashes, no deadlocks

✓ Validates: Long-term stability, no deadlocks, no resource leaks
```

### Test 5: Performance Measurement
**Purpose:** Verify minimal performance impact

```
1. Run 10,000 password attempts without protection mode
2. Measure: Time per attempt (baseline)
3. Enable protection mode with locks
4. Run 10,000 password attempts again
5. Measure: Time per attempt (with locks)
6. Verify: <1% performance degradation

Expected: Lock overhead is negligible (~20ns per acquisition)
```

---

## Edge Cases Handled

### 1. Map Iterator Invalidation
- **Risk**: Thread A erases entry while Thread B iterates map
- **Solution**: No iteration in code, only `count()`, `erase()`, `operator[]`
- **Result**: Safe - all operations protected by lock

### 2. TOCTOU (Time-of-Check Time-of-Use)
- **Risk**: Check blocked status, then decision becomes stale before use
- **Solution**: Check and action (return) happen in same critical section
- **Result**: Atomic check-and-act, no race window

### 3. Thread Termination While Holding Lock
- **Risk**: Thread crashes while holding lock → other threads deadlock forever
- **Mitigation**: Accept risk (educational project, no exceptions in code)
- **Future**: Add RAII wrapper for exception safety (C++ best practice)

### 4. GetTickCount() Wraparound
- **Risk**: GetTickCount() wraps after 49.7 days → blocks may fail
- **Not fixed in this PR**: Separate issue, low priority
- **Future**: Use `GetTickCount64()` which never wraps

### 5. Deadlock Risk
- **Risk**: Multiple locks acquired in different order → deadlock
- **Solution**: Only ONE lock used (`blockedUsersLock`) → deadlock impossible
- **Result**: Single lock design eliminates deadlock risk entirely

---

## Requirements.md Alignment

### Before Fix
| Requirement | Line | Status |
|-------------|------|--------|
| "Create at least 3 instances of Named Pipe" | 29 | ✅ COMPLETE (3 threads) |
| "Async/Overlapped mode recommended" | 29 | ❌ NOT IMPLEMENTED (optional) |
| "Delay must not freeze whole server" | 39-41 | 🔴 **BUG** (race breaks this) |
| "Anti-cracking mode (Delay logic)" | 82 | 🔴 **BUG** (not thread-safe) |
| "100% grade requires anti-cracking" | 82 | 🔴 **BLOCKED** by race |

### After Fix
| Requirement | Line | Status |
|-------------|------|--------|
| "Create at least 3 instances of Named Pipe" | 29 | ✅ COMPLETE |
| "Async/Overlapped mode recommended" | 29 | ❌ Optional (not critical) |
| "Delay must not freeze whole server" | 39-41 | ✅ **FIXED** (threads independent) |
| "Anti-cracking mode (Delay logic)" | 82 | ✅ **FIXED** (thread-safe) |
| "100% grade requires anti-cracking" | 82 | ✅ **COMPLETE** |

---

## Implementation Checklist

### Pre-Implementation
- [x] Confirm server has multithreading (3 threads) ✅
- [x] Confirm async I/O not required (optional per requirements) ✅
- [x] Identify race condition in list.h ✅
- [x] Design synchronization strategy ✅

### Code Changes (~30 lines, ~15 minutes)
- [ ] Add `CRITICAL_SECTION blockedUsersLock;` member to `UserList` class
- [ ] Initialize lock in constructor: `InitializeCriticalSection(&blockedUsersLock);`
- [ ] Add destructor: `DeleteCriticalSection(&blockedUsersLock);`
- [ ] Protect block check (lines 77-91) with `EnterCriticalSection` / `LeaveCriticalSection`
- [ ] Protect block clear on success (line 92) with lock/unlock
- [ ] Protect block insert on failure (line 98) with lock/unlock
- [ ] Add comments explaining lock scope
- [ ] Compile with no warnings

### Testing (~1-2 hours)
- [ ] Test 1: Single-client protection mode
- [ ] Test 2: Concurrent attacks on same login
- [ ] Test 3: Concurrent attacks on different logins
- [ ] Test 4: Stress test (1 hour continuous)
- [ ] Test 5: Performance measurement
- [ ] Verify no deadlocks
- [ ] Verify no crashes
- [ ] Document test results

---

## Rollback Plan

If issues occur during testing:

```cpp
// Revert to original CheckUser() (lines 75-105)
// Remove from class:
// - CRITICAL_SECTION blockedUsersLock;
// - InitializeCriticalSection() from constructor
// - Destructor with DeleteCriticalSection()
```

**Rollback Criteria:**
- Deadlocks detected (server hangs)
- Crashes increase compared to unlocked version
- Performance degradation >5%
- Unable to resolve bugs within 2 hours

**Mitigation:**
- Keep backup copy of `list.h` before changes
- Use version control (git commit before changes)
- Test in isolated environment first

---

## Summary

**What:** Fix critical race condition in server anti-cracking protection
**Why:** Required for 100% grade (requirements.md:39-41, 82)
**How:** Add `CRITICAL_SECTION` to synchronize `blockedUsers` map access
**Impact:** ~30 lines changed, <1% performance impact, eliminates crashes
**Time:** 30-45 min implementation + 1-2 hours testing = ~2-3 hours total
**Risk:** Low (standard pattern, single lock design prevents deadlocks)

**Grade Impact:**
- ✅ Enables 100% grade (fixes mandatory anti-cracking requirement)
- ✅ Meets requirements.md:39-41 (delay doesn't freeze server)
- ✅ Fixes undefined behavior (map corruption)
- ✅ Production-ready server implementation

---

## Alternative Implementation: Async/Overlapped I/O (Future Work)

While async I/O is **recommended** in requirements.md:29, it is **not mandatory** for grading. The current synchronous approach with 3 threads already meets all requirements.

If async I/O implementation is desired in the future, it would involve:

1. Add `FILE_FLAG_OVERLAPPED` to `CreateNamedPipe()` call
2. Use `OVERLAPPED` structures with `ReadFile` / `WriteFile`
3. Handle `ERROR_IO_PENDING` return codes
4. Use `GetOverlappedResult()` or `WaitForSingleObject()` to wait for I/O completion
5. Consider using **I/O Completion Ports (IOCP)** for better scalability

**Estimated effort:** 4-6 hours (more complex than race condition fix)

**Benefits:**
- Better scalability for >3 concurrent clients
- Industry best practice
- Reduced thread overhead

**Drawbacks:**
- More complex code
- Harder to debug
- Not required for grading

**Recommendation:** Fix race condition first (critical), consider async I/O later (optional improvement)
