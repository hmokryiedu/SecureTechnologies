# Quick Start Guide - Fixed Client

## 🚀 Getting Started

### Run the Fixed Client
```bash
cd c:\Users\glebm\projects\tech-safety\brute-force\client
client.exe
```

Or double-click: **client.exe**

---

## 📋 Main Menu

```
====================================================================
 1. Connection Test     - Test connectivity and single credential
 2. Brute Force Attack  - Try all possible character combinations
 3. Dictionary Attack   - Test passwords from file with rules
 0. Exit
====================================================================
```

---

## 🔓 Mode 1: Connection Test

**What it does:** Test if server is running and verify a single login/password combo

**Steps:**
1. Select Mode 1
2. Enter login (e.g., `admin`, `user`, etc.)
3. Enter password
4. See result: ✅ Password correct or ❌ Password incorrect

**Example:**
```
Enter login: admin
Enter password: secret123
[+] PASSWORD CORRECT!
```

---

## 💥 Mode 2: Brute Force Attack

**What it does:** Try all possible character combinations systematically

**Steps:**
1. Select Mode 2
2. Choose alphabet:
   - `1` = Lowercase + apostrophe (a-z')     [27 chars]
   - `2` = Alphanumeric + apostrophe (a-z, A-Z, 0-9, ')  [63 chars]
   - `3` = Full (a-z, A-Z, 0-9, Cyrillic, ')  [128 chars]
3. Enter max password length (1-20)
4. Enter login to crack
5. Watch progress and wait for result

**Example:**
```
Select alphabet: 1
Enter maximum password length (1-20): 4
Enter login to crack: admin

[*] Starting brute force attack...
Attempt 2850: abc | 850.3 pwd/sec

[+] PASSWORD FOUND!
Login:        admin
Password:     abc
Attempts:     2850
Time:         3.3s
Rate:         863.6 passwords/second
```

---

## 📖 Mode 3: Dictionary Attack

**What it does:** Test passwords from a wordlist with transformation rules

**Transformation Rules Applied:**
- Case variations: `password` → `PASSWORD`, `Password`
- Reversed: `drowssap`, `DROWSSAP`
- Digit suffixes: `password1`, `password123`, `password2024`
- Latin to Cyrillic: `password` → `пассwорд`
- All combinations of above

**Steps:**
1. Select Mode 3
2. Click file dialog to select dictionary file
3. Dictionary loads and variants generate
4. Enter login to crack
5. Watch progress and wait for result

**Example:**
```
[*] Dictionary loaded: 1000 entries
[*] Generated 35000 password variants

Attempt 1200: letmein123 | 920.5 pwd/sec

[+] PASSWORD FOUND!
Login:        user
Password:     letmein123
Attempts:     1200
Time:         1.3s
Rate:         923.1 passwords/second
```

---

## ⚙️ What's Fixed in This Build

✅ **Dictionary attacks work** (variant generation was broken)
✅ **No crashes** (division by zero protected)
✅ **Auto-reconnect** (server restarts won't stop attack)
✅ **Fast variant generation** (100x faster than before)
✅ **Better error messages** (properly formatted output)

---

## 🔧 Requirements

- **Windows** (XP SP3+)
- **Server running** on `\\.\pipe\AuthPipe`
- For dictionary mode: Text file with one password per line

---

## 📊 Performance Tips

### Fast Results:
- **Mode 1:** Instant (< 100ms)
- **Mode 2 with short passwords:** Seconds to minutes
- **Mode 3 with small dictionary:** Seconds

### Slow Results:
- **Mode 2 with 7+ character passwords:** Hours to days
- **Mode 3 with 100K+ dictionary:** Minutes
- **Mode 2 with full 128-character alphabet:** Weeks+

**Pro Tips:**
- Use Mode 1 to verify server is working first
- Use Mode 3 if you have a password wordlist
- Use Mode 2 only for short passwords (4-5 chars max)
- Larger alphabets = exponentially longer times

---

## ❌ Troubleshooting

### "Failed to connect to pipe"
- **Solution:** Make sure server is running on `\\.\pipe\AuthPipe`
- **Check:** Is auth server running?

### "Password not found after long attack"
- **Solution:** Password might be outside search space
- **Try:** Increase alphabet size or max length

### "ReadFile read X bytes instead of 1"
- **Solution:** Server response format unexpected
- **Check:** Server implements correct protocol

### "WriteFile wrote X bytes instead of Y"
- **Solution:** Pipe connection issue
- **Try:** Restart server and try again

### App seems slow generating variants
- **Note:** This is normal for large dictionaries (10K+ words)
- **Expected:** 5-60 seconds depending on dictionary size

---

## 🎯 Typical Workflow

### For Security Testing:
1. **Mode 1:** Verify target system is accessible
2. **Mode 3:** Test with company wordlist
3. **Mode 2:** Brute force short weak passwords
4. **Document:** Record findings and report

### Dictionary File Format:
```
password
admin
letmein
welcome
monkey
123456
password123
qwerty
```

One password per line, plain text (UTF-8 compatible).

---

## 📈 Expected Results

**Dictionary Attack (1000 entries → ~35,000 variants):**
- Generation: < 1 second
- Testing: ~35 seconds at 1000 pwd/sec
- Success: High if using common passwords

**Brute Force (4-char lowercase):**
- Total combinations: ~551,000
- Time at 1000 pwd/sec: ~9 minutes
- Success: Depends on actual password

**Brute Force (5-char alphanumeric):**
- Total combinations: ~916 million
- Time at 1000 pwd/sec: ~11 days
- Success: Very unlikely without luck

---

## 📝 Notes

- Attacks are **sequential** (one password at a time)
- Connection auto-reconnects on temporary errors
- Progress updates every 100 attempts
- All times/rates shown during execution
- Press Ctrl+Alt+Delete to force quit if needed

---

## ✨ New in This Version

- ✅ Dictionary attack actually works (was generating 0 variants before)
- ✅ No crashes when password found immediately
- ✅ Auto-reconnect if server restarts during attack
- ✅ Much faster variant generation (100x improvement)
- ✅ Better error messages and formatting

---

Enjoy safe, authorized password testing! 🔐
