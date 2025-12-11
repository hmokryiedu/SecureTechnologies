# Task: Create Vocabulary Files for Rule-Based Password Attacks

## Overview
According to **requirements.md Section 3.B.3**, the client application must support **Rule-Based Attacks** that load "Probable Passwords" from a file. These vocabulary files are **required for achieving 100% grade**.

**Current Status:** ❌ **NOT IMPLEMENTED** - No vocabulary files exist in the project.

---

## Requirements from requirements.md

### Section 3.B.3: Rule-Based Attack Configuration (100% Grade)
> * Load "Probable Passwords" from a file via `GetOpenFileName`.
> * Apply heuristic rules (implemented as class methods):
>     * Reverse string.
>     * Change case.
>     * Keyboard layout swap (e.g., typing "ghbdtn" instead of "привет").
>     * Transposition of characters.

### Section 5: The Scenario (Data)
> * **Password Characteristics:**
>     1. Violates "Don't use common sequences" (e.g., "123", "qwerty").
>     2. Violates "Don't use personal info" (dates, names).
>     3. Violates "Don't use short passwords".
>     4. Strong password (follows all rules).

---

## Purpose of Vocabulary Files

Vocabulary files contain **base passwords** (also called "probable passwords" or "dictionary words") that serve as the starting point for rule-based attacks. The rule-based attack engine:

1. **Loads** base passwords from the vocabulary file
2. **Applies transformation rules** to each base password:
   - Reverse the string
   - Change case variations (lowercase, UPPERCASE, Capitalized)
   - Swap keyboard layout (Latin ↔ Cyrillic)
   - Transpose adjacent characters
3. **Tests** each generated variant against the server

This approach is **much faster** than brute force because it targets common password patterns.

---

## How Vocabulary Files Are Used

### Client Application Flow (RuleBasedAttack Mode)

```
┌─────────────────────────────────────────────────────────────┐
│ 1. User selects "Rule-Based Attack" from main menu          │
└────────────────┬────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────┐
│ 2. GetOpenFileName() dialog prompts for vocabulary file     │
│    Example: "Select vocabulary file (*.txt)"                │
└────────────────┬────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────┐
│ 3. RuleBasedAttack::LoadDictionaryFromFile(filepath)        │
│    - Reads file line by line                                │
│    - Each line = one base password                          │
│    - Stores in LinkedList<std::string> dictionary           │
└────────────────┬────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────┐
│ 4. RuleBasedAttack::ApplyAllRules()                         │
│    For each base password:                                  │
│      - Generate all rule variants (reverse, case, etc.)     │
│      - Store unique variants in LinkedList                  │
└────────────────┬────────────────────────────────────────────┘
                 │
                 v
┌─────────────────────────────────────────────────────────────┐
│ 5. RuleBasedAttack::Start(login)                            │
│    - Connect to server via Named Pipe                       │
│    - Try each variant sequentially                          │
│    - Display current attempt and elapsed time               │
│    - Stop when password is cracked                          │
└─────────────────────────────────────────────────────────────┘
```

**Code Reference:** `client/RuleAttack.cpp:60-73` (LoadDictionaryFromFile method)

---

## Vocabulary File Format Specification

### Basic Format
```
Plain text file (.txt extension)
UTF-8 encoding (required for Cyrillic support)
One password per line
No headers or metadata
Unix (LF) or Windows (CRLF) line endings accepted
Empty lines are ignored
```

### Example Structure

**File:** `weak_passwords.txt`
```
123
qwerty
password
admin
12345678
welcome
letmein
monkey
dragon
master
```

**File:** `russian_common.txt` (UTF-8 encoded)
```
привет
пароль
admin
qwerty
12345
солнце
любовь
медведь
```

**File:** `personal_info.txt`
```
Winter
Lab2025
Sarah
John
Moscow
Birthday
2000
1985
MyDog
```

---

## Required Vocabulary Files to Create

To test all password violation scenarios from requirements.md Section 5, create **4 vocabulary files**:

### 1. `vocab_common_sequences.txt` - Tests Password #1 (Common Sequences)
**Purpose:** Crack passwords that violate "Don't use common sequences"

**Content Examples:**
```
123
1234
12345
123456
qwerty
qwertyuiop
asdfgh
abc
password
admin
root
letmein
welcome
monkey
dragon
master
trustno1
```

**Expected Target:** `viol_R1: Winter123` (contains "123" sequence)

---

### 2. `vocab_personal_info.txt` - Tests Password #2 (Personal Info)
**Purpose:** Crack passwords based on dates, names, common words

**Content Examples:**
```
winter
spring
summer
autumn
lab
2025
2024
2023
january
february
moscow
sarah
john
home
family
dog
cat
password
admin
```

**Expected Target:** `viol_R2: Lab2025!` (contains "Lab" + "2025")

**Note:** Rule engine will apply case variations (Winter, WINTER, winter) and combine with numbers.

---

### 3. `vocab_short_words.txt` - Tests Password #3 (Short Passwords)
**Purpose:** Crack short passwords (< 8 characters)

**Content Examples:**
```
my
hi
ok
yes
no
dog
cat
sun
key
run
win
lab
123
abc
qwe
asd
pass
user
root
me
```

**Expected Target:** `viol_R5: MyP!` (short password, 4 chars)

**Note:** Very short base words allow rule engine to test all combinations quickly.

---

### 4. `vocab_russian_cyrillic.txt` - Tests Keyboard Layout Swap
**Purpose:** Test Cyrillic/Latin keyboard layout swap rule

**Content Examples (UTF-8 encoding required):**
```
привет
пароль
admin
qwerty
солнце
любовь
медведь
россия
москва
зима
лето
собака
кошка
дом
семья
```

**How it works:**
- Base word: `привет` (Russian for "hello")
- LatinToCyrillic rule transforms Latin typing errors
- User types "ghbdtn" on Latin keyboard → system checks if it matches "привет"

**Expected Target:** Depends on server configuration (may test Cyrillic passwords)

---

## Implementation Steps

### Step 1: Create Vocabulary Directory
```bash
mkdir vocabularies
cd vocabularies
```

### Step 2: Create Each Vocabulary File

**File 1:** `vocabularies/vocab_common_sequences.txt`
- Add 50-100 common password sequences
- Include numeric patterns (123, 1234, 12345678)
- Include keyboard patterns (qwerty, asdfgh, zxcvbn)
- Include dictionary words (password, admin, welcome)

**File 2:** `vocabularies/vocab_personal_info.txt`
- Add 50-100 words related to personal information
- Include seasons (winter, summer)
- Include years (2020-2025)
- Include common names (john, sarah, alex)
- Include locations (moscow, london, home)

**File 3:** `vocabularies/vocab_short_words.txt`
- Add 100-200 short words (1-5 characters)
- Include common abbreviations (my, hi, ok)
- Include short numbers (1, 12, 123)
- Include single letters and combinations

**File 4:** `vocabularies/vocab_russian_cyrillic.txt` (UTF-8)
- Add 50-100 common Russian words
- Use proper UTF-8 encoding in text editor
- Test with Cyrillic keyboard layout swap rule

### Step 3: Validate File Format
```bash
# Check encoding (should be UTF-8)
file -i vocab_russian_cyrillic.txt

# Check line count
wc -l vocab_common_sequences.txt

# Verify no empty lines or special characters
cat vocab_common_sequences.txt | grep -v '^$'
```

### Step 4: Test with Client Application

**Testing Procedure:**
1. Run `client.exe`
2. Select option **"3. Rule-Based Attack Mode"** (adjust based on actual menu)
3. When prompted by GetOpenFileName dialog:
   - Navigate to `vocabularies/` folder
   - Select `vocab_common_sequences.txt`
4. Enter target login: `viol_R1`
5. Observe attack progress
6. Verify password `Winter123` is cracked

**Expected Results:**
- `vocab_common_sequences.txt` + rules → Cracks `Winter123` (contains "123")
- `vocab_personal_info.txt` + rules → Cracks `Lab2025!` (contains "Lab", "2025")
- `vocab_short_words.txt` + rules → Cracks `MyP!` (short password)

---

## File Size Recommendations

### Small Vocabulary (Testing Phase)
- **Size:** 50-100 base passwords per file
- **Total variants after rules:** ~500-1,000 attempts per file
- **Attack time:** 10-60 seconds (depending on server delay)
- **Use case:** Quick testing, debugging rule engine

### Medium Vocabulary (Lab Report)
- **Size:** 500-1,000 base passwords per file
- **Total variants after rules:** ~5,000-10,000 attempts per file
- **Attack time:** 1-10 minutes
- **Use case:** Demonstrate attack effectiveness for lab report

### Large Vocabulary (Real-World Simulation)
- **Size:** 10,000+ base passwords per file
- **Total variants after rules:** ~100,000+ attempts per file
- **Attack time:** Hours (depends on multithreading)
- **Use case:** Stress testing, performance analysis

**Recommended for this lab:** Start with **Small Vocabulary** (50-100 words) to verify functionality, then expand to **Medium Vocabulary** for final report.

---

## Where to Find Real Password Dictionaries

For more realistic testing, you can download open-source password lists:

### Recommended Sources:
1. **SecLists** (Daniel Miessler)
   - https://github.com/danielmiessler/SecLists
   - Path: `Passwords/Common-Credentials/`
   - Files: `10-million-password-list-top-100.txt`, `10k-most-common.txt`

2. **RockYou Leak** (Cleaned subset)
   - Famous password breach dataset
   - Available in SecLists or security research repositories
   - **Warning:** Only use for educational purposes

3. **Custom Russian Dictionaries**
   - Search for "russian common passwords github"
   - Ensure UTF-8 encoding
   - Combine with keyboard layout swap rule

**Important:** For this lab, manually creating small custom vocabularies is **recommended** to demonstrate understanding of password weaknesses.

---

## Example: How Rules Transform Base Passwords

### Base Password: `winter`

**Applied Rules (from RuleAttack.cpp):**

1. **Original:** `winter`
2. **Reverse:** `retniw`
3. **Lowercase:** `winter` (same)
4. **Uppercase:** `WINTER`
5. **Capitalize:** `Winter`
6. **LatinToCyrillic:** `цинтер` (phonetic approximation)
7. **Transposition (adjacent swaps):**
   - `iwnter` (swap w↔i)
   - `wniter` (swap n↔i)
   - `winetr` (swap t↔e)
   - `wintær` (swap e↔r)

**Total Variants from 1 Base Word:** ~10-15 variants

**With Dictionary of 100 Words:** ~1,000-1,500 total password attempts

**Why This Works:**
- Real users often:
  - Capitalize first letter: `Winter` instead of `winter`
  - Add numbers: `Winter123` (tested by combining with sequences)
  - Make typos: Transposition rule catches common typing errors

---

## Testing Checklist

- [ ] Create `vocabularies/` directory
- [ ] Create `vocab_common_sequences.txt` with 50+ entries
- [ ] Create `vocab_personal_info.txt` with 50+ entries
- [ ] Create `vocab_short_words.txt` with 100+ entries
- [ ] Create `vocab_russian_cyrillic.txt` with UTF-8 encoding
- [ ] Verify all files use UTF-8 encoding
- [ ] Verify all files have one password per line
- [ ] Test file loading with client application
- [ ] Verify GetOpenFileName dialog appears correctly
- [ ] Test successful password cracking with each vocabulary
- [ ] Measure attack time for lab report
- [ ] Compare rule-based attack vs brute force timing

---

## Integration with Current Implementation

### Current Code (From project-overview.md)

**File:** `client/RuleAttack.cpp:60-73`
```cpp
void RuleBasedAttack::LoadDictionaryFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Failed to open dictionary file: " << filename << std::endl;
        return;
    }
    std::string word;
    while (std::getline(file, word)) {
        if (!word.empty()) {
            dictionary.push_back(word);  // Currently uses std::vector
        }
    }
    file.close();
    std::cout << "Loaded " << dictionary.size() << " words from dictionary." << std::endl;
}
```

**This code already supports vocabulary file loading!** Just needs vocabulary files to be created.

---

## Expected Lab Report Results

With properly created vocabulary files, you should be able to report:

### Attack Success Rate
| Password Type | Vocabulary File | Attack Success | Time |
|---------------|----------------|----------------|------|
| viol_R1 (Winter123) | vocab_common_sequences.txt | ✓ Yes | ~30 sec |
| viol_R2 (Lab2025!) | vocab_personal_info.txt | ✓ Yes | ~45 sec |
| viol_R5 (MyP!) | vocab_short_words.txt | ✓ Yes | ~10 sec |
| ideal_R4 (4Dogs@Home) | All vocabularies combined | ✗ No | N/A |

### Analysis Points for Report
1. **Rule-based attacks are 10-1000x faster** than brute force for weak passwords
2. **Strong passwords resist dictionary attacks** (ideal_R4 should NOT be cracked)
3. **Keyboard layout swap is effective** for bilingual users who make typing mistakes
4. **Case variations catch ~80% of weak passwords** (users often just capitalize first letter)
5. **Multithreading provides X% speedup** (test with client multithreading implementation)

---

## Priority

**Urgency:** HIGH
**Grade Impact:** Required for 100% grade (rule-based attack functionality)
**Estimated Time:** 1-2 hours (including testing)

---

## Summary

### What to Create:
1. ✅ **vocabularies/** directory
2. ✅ **vocab_common_sequences.txt** - 50-100 common patterns
3. ✅ **vocab_personal_info.txt** - 50-100 personal info words
4. ✅ **vocab_short_words.txt** - 100-200 short words
5. ✅ **vocab_russian_cyrillic.txt** - 50-100 Cyrillic words (UTF-8)

### How to Use:
1. Run `client.exe` → Select "Rule-Based Attack Mode"
2. Use GetOpenFileName dialog to select vocabulary file
3. Enter target login
4. Watch rule engine generate and test variants
5. Measure time for lab report

### File Format:
- Plain text, UTF-8 encoding
- One password per line
- No headers, no empty lines
- Works with existing LoadDictionaryFromFile() code

### Expected Results:
- Crack 3/4 weak passwords in under 2 minutes each
- Strong password remains secure (proves protection works)
- Demonstrate rule-based attack effectiveness for lab report

---

## Next Steps After Creating Vocabularies

1. **Test each vocabulary file** individually
2. **Combine vocabularies** into one large file for comprehensive testing
3. **Measure timing metrics** for lab report graphs
4. **Compare with brute force** to show efficiency improvement
5. **Test with anti-cracking protection enabled** on server
6. **Document results** in final lab report with screenshots
