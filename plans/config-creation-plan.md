# Rule-Based Attack Configuration Implementation Plan

## Executive Summary

**Objective**: Implement the ONLY missing requirement for 100% grade - Rule-Based Attack Configuration File

**Requirements Source**: requirements.pdf Page 4
> "Для каждого типа правила должен быть указан список наиболее вероятных паролей, который задается именем файла"

**Estimated Effort**: 5-6 hours
**Files to Modify**: 3 (RuleAttack.h, RuleAttack.cpp, main.cpp)
**New Classes**: RuleSet struct, enhanced RuleBasedAttack class

---

## Code Quality Requirements

### OOP Principles to Follow

1. **Single Responsibility Principle (SRP)**
   - Each class/method has ONE clear purpose
   - `ConfigParser` → parses config files
   - `RuleSet` → stores rule+vocabulary pair
   - `ApplyRule()` → applies ONE specific rule

2. **Encapsulation**
   - Keep implementation details private
   - Expose only necessary public interfaces
   - Use private helpers for internal logic

3. **RAII (Resource Acquisition Is Initialization)**
   - Files closed automatically (ifstream destructor)
   - No manual resource management needed
   - LinkedList handles its own memory

4. **Const Correctness**
   - Use `const` for read-only operations
   - Pass by `const reference` when not modifying
   - Mark methods `const` when they don't modify state

5. **Clear Naming**
   - `LoadConfigFile()` - verb + noun, clear purpose
   - `RuleType` - enum, not int
   - `ruleSets` - plural, indicates collection

### Best Practices to Apply

1. **Error Handling**
   - Check all file operations for failure
   - Provide clear, actionable error messages
   - Don't silently fail - log warnings for skipped entries

2. **Code Readability**
   - Maximum 50 lines per method
   - Meaningful variable names (no `x`, `tmp`, `data`)
   - Comments explain WHY, not WHAT

3. **DRY (Don't Repeat Yourself)**
   - Extract common patterns into helper methods
   - Reuse existing code (GenerateCaseVariations, etc.)
   - No copy-paste programming

4. **Defensive Programming**
   - Validate inputs before use
   - Handle edge cases (empty files, invalid data)
   - Clear error messages with context

---

## Implementation Design

### Class Structure

```cpp
// Clean OOP design
enum class RuleType {
    CASE_VARIATIONS,
    DIGIT_SUFFIXES,
    CYRILLIC_LAYOUT,
    CHARACTER_TRANSPOSE,
    STRING_REVERSAL,
    LATIN_TO_CYRILLIC
};

struct RuleSet {
    RuleType type;
    LinkedList<std::string> vocabulary;

    explicit RuleSet(RuleType ruleType) : type(ruleType) {}

    // Rule of Five: explicitly default/delete as needed
    RuleSet(const RuleSet&) = default;
    RuleSet& operator=(const RuleSet&) = default;
    RuleSet(RuleSet&&) = default;
    RuleSet& operator=(RuleSet&&) = default;
    ~RuleSet() = default;
};

class RuleBasedAttack : public PasswordGenerator {
private:
    // Data members
    LinkedList<RuleSet> m_ruleSets;
    LinkedList<std::string> m_variants;
    std::unordered_set<std::string> m_variantSet;
    size_t m_currentIndex;

    // Helper methods (private implementation)
    RuleType ParseRuleType(const std::string& ruleString) const;
    bool LoadVocabularyFile(const std::string& filename, LinkedList<std::string>& vocabulary) const;
    void ApplySpecificRule(RuleType rule, const std::string& word);
    void AddVariant(const std::string& variant);

public:
    // Public interface
    RuleBasedAttack();

    bool LoadConfigFile(const std::string& configPath);
    void GenerateVariants() override;

    // Inherited from PasswordGenerator
    std::string Next() override;
    bool HasNext() const override;
    size_t GetVariantCount() const;
};
```

### Configuration File Format

**File**: `attack_config.txt`

```
# Rule-Based Attack Configuration
# Format: RULE_TYPE vocabulary_file_path
# Lines starting with # are comments
# Blank lines are ignored

# Apply case variations to personal information words
CASE_VARIATIONS vocabularies/vocab_personal_info.txt

# Add digit suffixes to short common words
DIGIT_SUFFIXES vocabularies/vocab_short_words.txt

# Convert Cyrillic to Latin keyboard layout
CYRILLIC_LAYOUT vocabularies/vocab_russian_cyrillic.txt

# Transpose adjacent characters to catch typos
CHARACTER_TRANSPOSE vocabularies/vocab_common_sequences.txt

# Reverse strings
STRING_REVERSAL vocabularies/vocab_personal_info.txt

# Convert Latin to Cyrillic (phonetic)
LATIN_TO_CYRILLIC vocabularies/vocab_common_sequences.txt
```

---

## Step-by-Step Implementation

### Step 1: Add Enumerations and Structures (30 min)

**File**: `client/RuleAttack.h`

**Changes**:
1. Add enum class for rule types (type-safe)
2. Add RuleSet struct (encapsulates rule+vocabulary)
3. Add private member `m_ruleSets`
4. Add public method `LoadConfigFile()`
5. Add private helpers for parsing and rule application

**Code Quality Focus**:
- Use `enum class` instead of plain `enum` (type safety)
- Prefix members with `m_` (clear distinction from parameters)
- Make helpers private (encapsulation)
- Use `const` for read-only methods

### Step 2: Implement Config File Parser (1.5 hours)

**File**: `client/RuleAttack.cpp`

```cpp
bool RuleBasedAttack::LoadConfigFile(const std::string& configPath) {
    // Open file
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        Console::PrintError("Failed to open configuration file: " + configPath);
        return false;
    }

    // Clear previous state
    m_ruleSets.clear();
    m_variants.clear();
    m_variantSet.clear();
    m_currentIndex = 0;

    // Parse file line by line
    std::string line;
    size_t lineNumber = 0;

    while (std::getline(configFile, line)) {
        lineNumber++;

        // Trim whitespace
        const std::string trimmed = TrimWhitespace(line);

        // Skip empty lines and comments
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        // Parse line: RULE_TYPE vocabulary_file_path
        std::istringstream lineStream(trimmed);
        std::string ruleTypeString, vocabularyPath;

        if (!(lineStream >> ruleTypeString >> vocabularyPath)) {
            Console::PrintWarning("Line " + std::to_string(lineNumber) +
                                 ": Invalid format, expected RULE_TYPE vocabulary_file_path");
            continue;
        }

        // Parse rule type
        const RuleType ruleType = ParseRuleType(ruleTypeString);
        if (ruleType == RuleType::INVALID) {
            Console::PrintWarning("Line " + std::to_string(lineNumber) +
                                 ": Unknown rule type '" + ruleTypeString + "'");
            continue;
        }

        // Create and load rule set
        RuleSet ruleSet(ruleType);
        if (!LoadVocabularyFile(vocabularyPath, ruleSet.vocabulary)) {
            Console::PrintWarning("Line " + std::to_string(lineNumber) +
                                 ": Failed to load vocabulary from '" + vocabularyPath + "'");
            continue;
        }

        // Add to rule sets
        m_ruleSets.push_back(std::move(ruleSet));  // Use move semantics

        Console::PrintInfo("Loaded " + ruleTypeString + ": " +
                          std::to_string(ruleSet.vocabulary.size()) +
                          " words from " + vocabularyPath);
    }

    // Validate that at least one rule was loaded
    if (m_ruleSets.size() == 0) {
        Console::PrintError("No valid rules found in configuration file!");
        return false;
    }

    Console::PrintSuccess("Configuration loaded: " +
                         std::to_string(m_ruleSets.size()) + " rule sets");
    return true;
}

// Helper: Trim whitespace (clean utility function)
std::string RuleBasedAttack::TrimWhitespace(const std::string& str) const {
    const size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";  // All whitespace
    }

    const size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

// Helper: Parse rule type string (encapsulated mapping logic)
RuleType RuleBasedAttack::ParseRuleType(const std::string& ruleString) const {
    static const std::map<std::string, RuleType> ruleMap = {
        {"CASE_VARIATIONS", RuleType::CASE_VARIATIONS},
        {"DIGIT_SUFFIXES", RuleType::DIGIT_SUFFIXES},
        {"CYRILLIC_LAYOUT", RuleType::CYRILLIC_LAYOUT},
        {"CHARACTER_TRANSPOSE", RuleType::CHARACTER_TRANSPOSE},
        {"STRING_REVERSAL", RuleType::STRING_REVERSAL},
        {"LATIN_TO_CYRILLIC", RuleType::LATIN_TO_CYRILLIC}
    };

    const auto it = ruleMap.find(ruleString);
    return (it != ruleMap.end()) ? it->second : RuleType::INVALID;
}

// Helper: Load vocabulary file (SRP - single responsibility)
bool RuleBasedAttack::LoadVocabularyFile(const std::string& filename,
                                         LinkedList<std::string>& vocabulary) const {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string word;
    while (std::getline(file, word)) {
        const std::string trimmed = TrimWhitespace(word);
        if (!trimmed.empty()) {
            vocabulary.push_back(trimmed);
        }
    }

    return vocabulary.size() > 0;
}
```

**Code Quality Features**:
- ✅ Clear method responsibilities (SRP)
- ✅ Const correctness (`const` methods, `const std::string&`)
- ✅ Move semantics for efficiency (`std::move(ruleSet)`)
- ✅ Descriptive error messages with context
- ✅ Helper methods for common operations (TrimWhitespace)
- ✅ Static map for rule type parsing (initialized once)
- ✅ Early returns for error cases (guard clauses)

### Step 3: Modify Rule Application Logic (1.5 hours)

**File**: `client/RuleAttack.cpp`

```cpp
// Public method: Generate all variants from loaded rule sets
void RuleBasedAttack::GenerateVariants() {
    // Clear previous results
    m_variants.clear();
    m_variantSet.clear();

    // Apply each rule to its corresponding vocabulary
    for (auto ruleIt = m_ruleSets.begin(); ruleIt != m_ruleSets.end(); ++ruleIt) {
        const RuleSet& ruleSet = *ruleIt;

        for (auto wordIt = ruleSet.vocabulary.begin();
             wordIt != ruleSet.vocabulary.end();
             ++wordIt) {

            ApplySpecificRule(ruleSet.type, *wordIt);
        }
    }

    Console::PrintInfo("Generated " + std::to_string(m_variants.size()) +
                      " unique password variants");
}

// Private method: Apply specific rule to specific word (clean switch)
void RuleBasedAttack::ApplySpecificRule(RuleType rule, const std::string& word) {
    switch (rule) {
        case RuleType::CASE_VARIATIONS:
            GenerateCaseVariations(word);
            break;

        case RuleType::DIGIT_SUFFIXES:
            GenerateDigitSuffixes(word);
            break;

        case RuleType::CYRILLIC_LAYOUT: {
            const std::string latinVersion = CyrillicToLatin(word);
            if (latinVersion != word) {
                GenerateCaseVariations(latinVersion);
                GenerateDigitSuffixes(latinVersion);
            }
            break;
        }

        case RuleType::CHARACTER_TRANSPOSE: {
            const auto transposedWords = TransposeCharacters(word);
            for (const auto& transposed : transposedWords) {
                GenerateCaseVariations(transposed);
            }
            break;
        }

        case RuleType::STRING_REVERSAL: {
            const std::string reversed = ReversePassword(word);
            GenerateCaseVariations(reversed);
            GenerateDigitSuffixes(reversed);
            break;
        }

        case RuleType::LATIN_TO_CYRILLIC: {
            const std::string cyrillicVersion = LatinToCyrillic(word);
            if (cyrillicVersion != word) {
                GenerateCaseVariations(cyrillicVersion);
                GenerateDigitSuffixes(cyrillicVersion);
            }
            break;
        }

        case RuleType::INVALID:
            // Should never happen (defensive programming)
            Console::PrintWarning("Attempted to apply invalid rule type");
            break;
    }
}

// Private helper: Add variant with deduplication (encapsulated logic)
void RuleBasedAttack::AddVariant(const std::string& variant) {
    // Check if already exists (O(1) lookup)
    if (m_variantSet.find(variant) == m_variantSet.end()) {
        m_variants.push_back(variant);
        m_variantSet.insert(variant);
    }
}
```

**Code Quality Features**:
- ✅ Const references prevent unnecessary copies
- ✅ Descriptive local variable names (`latinVersion`, not `tmp`)
- ✅ Scope minimization (variables declared in narrowest scope)
- ✅ Defensive programming (handle INVALID case)
- ✅ Encapsulated deduplication logic (AddVariant helper)
- ✅ Clear method purpose (one rule per call)

### Step 4: Update Client Menu (30 min)

**File**: `client/main.cpp`

```cpp
void ModeRuleBasedAttack() {
    Console::PrintHeader("DICTIONARY ATTACK WITH RULES");

    RuleBasedAttack attack;

    // Get configuration file from user
    const std::string configPath = GetConfigFilePath();
    if (configPath.empty()) {
        Console::PrintWarning("Configuration file selection cancelled.");
        return;
    }

    // Load configuration
    if (!attack.LoadConfigFile(configPath)) {
        Console::PrintError("Failed to load configuration. Attack aborted.");
        return;
    }

    // Generate password variants
    attack.GenerateVariants();

    if (!attack.HasNext()) {
        Console::PrintWarning("No variants generated. Check configuration.");
        return;
    }

    // Get target login
    std::string targetLogin;
    Console::PrintInfo("Enter target login: ");
    std::getline(std::cin, targetLogin);

    if (targetLogin.empty()) {
        Console::PrintError("Login cannot be empty!");
        return;
    }

    // Execute attack
    PipeClient client;
    Timer timer;
    timer.Start();

    size_t attemptCount = 0;
    bool passwordFound = false;
    std::string foundPassword;

    while (attack.HasNext()) {
        const std::string password = attack.Next();
        attemptCount++;

        if (client.TryPassword(targetLogin, password)) {
            passwordFound = true;
            foundPassword = password;
            break;
        }

        // Progress update
        if (attemptCount % 100 == 0) {
            Console::PrintInfo("Attempt " + std::to_string(attemptCount) +
                             ": " + password);
        }
    }

    timer.Stop();

    // Display results
    DisplayAttackResults(passwordFound, targetLogin, foundPassword,
                        attemptCount, timer.GetElapsed());
}

// Helper: Get config file path (extracted for clarity)
std::string GetConfigFilePath() {
    OPENFILENAMEA ofn;
    char filename[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "Config Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select Rule-Based Attack Configuration";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameA(&ofn)) {
        return "";  // User cancelled
    }

    return std::string(filename);
}

// Helper: Display attack results (extracted for clarity)
void DisplayAttackResults(bool success, const std::string& login,
                         const std::string& password,
                         size_t attempts, double elapsedMs) {
    Console::PrintSeparator('=', 60);

    if (success) {
        Console::PrintSuccess("PASSWORD FOUND!");
        std::cout << "Login:    " << login << "\n";
        std::cout << "Password: " << password << "\n";
    } else {
        Console::PrintError("Password not found.");
    }

    std::cout << "Attempts: " << attempts << "\n";
    std::cout << "Time:     " << (elapsedMs / 1000.0) << " seconds\n";
    std::cout << "Rate:     " << (attempts / (elapsedMs / 1000.0)) << " pwd/sec\n";

    Console::PrintSeparator('=', 60);
}
```

**Code Quality Features**:
- ✅ Extracted helpers (GetConfigFilePath, DisplayAttackResults)
- ✅ Const variables where appropriate
- ✅ Early returns for error cases
- ✅ Clear, self-documenting code
- ✅ No magic numbers (100 for progress is clearly progress interval)
- ✅ Descriptive function names

---

## Testing Strategy

### Unit Tests (Manual)

1. **Config Parser Tests**
   ```
   Test 1: Valid config with all rule types → Success
   Test 2: Empty config file → Error message
   Test 3: Config with comments and blank lines → Ignore them
   Test 4: Invalid rule type → Warning, skip line
   Test 5: Missing vocabulary file → Warning, skip rule
   Test 6: Vocabulary file with empty lines → Skip empty lines
   ```

2. **Rule Application Tests**
   ```
   Test 1: CASE_VARIATIONS "test" → test, Test, TEST
   Test 2: DIGIT_SUFFIXES "pass" → pass1, pass12, pass123, etc.
   Test 3: CYRILLIC_LAYOUT "привет" → ghbdtn
   Test 4: CHARACTER_TRANSPOSE "abc" → abc, bac, acb
   Test 5: STRING_REVERSAL "test" → tset
   Test 6: LATIN_TO_CYRILLIC "test" → тест
   ```

3. **Integration Tests**
   ```
   Test 1: Full attack with known password → Found
   Test 2: Attack with unknown password → Not found
   Test 3: Multiple rule sets → All applied correctly
   Test 4: Duplicate variants → Removed
   ```

### Edge Cases

1. Config file doesn't exist → Clear error
2. Vocabulary file doesn't exist → Skip rule with warning
3. Config with duplicate rules → Both applied
4. Empty vocabulary file → Skip rule
5. Very long lines in config → Handled correctly

---

## Code Review Checklist

Before submitting, verify:

### OOP Design
- [ ] Classes have single, clear responsibilities
- [ ] Data members are private
- [ ] Public interface is minimal and clear
- [ ] No unnecessary getters/setters
- [ ] Proper const correctness

### Code Quality
- [ ] No magic numbers (use named constants)
- [ ] Meaningful variable names (no `i`, `x`, `tmp` except loop counters)
- [ ] Functions < 50 lines
- [ ] No code duplication
- [ ] Comments explain WHY, not WHAT

### Error Handling
- [ ] All file operations checked
- [ ] Clear error messages with context
- [ ] No silent failures
- [ ] Defensive programming for edge cases

### Performance
- [ ] Pass by const reference where appropriate
- [ ] Use move semantics when transferring ownership
- [ ] No unnecessary copies
- [ ] Efficient algorithms (O(1) deduplication)

### Maintainability
- [ ] Code is self-documenting
- [ ] Easy to add new rule types
- [ ] Easy to modify config format
- [ ] Clear separation of concerns

---

## Files to Modify

| File | Purpose | Lines Changed |
|------|---------|---------------|
| `client/RuleAttack.h` | Add enum, struct, methods | +40 |
| `client/RuleAttack.cpp` | Implement parser, rules | +150 |
| `client/main.cpp` | Update menu, add helpers | +80 |
| **Total** | | **~270 lines** |

---

## Time Breakdown

| Task | Time | Focus |
|------|------|-------|
| Step 1: Design & Headers | 30 min | Clean OOP design |
| Step 2: Config Parser | 1.5 hr | Error handling, validation |
| Step 3: Rule Application | 1.5 hr | Clean switch, const correctness |
| Step 4: Menu Integration | 30 min | Helper extraction, clarity |
| Testing & Debugging | 1 hr | Edge cases, validation |
| Code Review & Polish | 30 min | Comments, cleanup |
| **TOTAL** | **5-6 hours** | |

---

## Success Criteria

Implementation is complete when:

1. ✅ Config file loads via GetOpenFileName
2. ✅ Rules mapped to vocabularies correctly
3. ✅ Variants generated without duplicates
4. ✅ Attack finds passwords successfully
5. ✅ Error messages are clear and actionable
6. ✅ Code follows OOP principles
7. ✅ Code is clean, readable, maintainable
8. ✅ All edge cases handled gracefully
9. ✅ Uses LinkedList for all collections
10. ✅ No memory leaks or resource leaks

---

## Maintenance Considerations

### Adding New Rule Types

Easy to extend (Open/Closed Principle):

1. Add new value to `RuleType` enum
2. Add mapping in `ParseRuleType()`
3. Add case in `ApplySpecificRule()`
4. Document in config file

### Modifying Config Format

If format needs to change:
- Only modify `LoadConfigFile()`
- Parser logic encapsulated
- Rest of code unaffected

### Performance Optimization

If attack speed is too slow:
- Profile first (don't guess)
- Consider parallel rule application
- Optimize deduplication if needed

---

## Summary

**What**: Implement rule-based attack configuration file
**Why**: Required by requirements.pdf for 100% grade
**How**: Clean OOP design with proper encapsulation
**Effort**: 5-6 hours of focused development
**Result**: 100% requirements compliance

**Key Principles**:
- Clean, readable code
- Proper OOP design
- Comprehensive error handling
- Easy to maintain and extend
- Well-tested edge cases

