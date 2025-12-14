#include "RuleAttack.h"
#include <windows.h>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <set>

// Latin to Cyrillic phonetic mapping
const std::map<char, std::string> RuleBasedAttack::latinToCyrillic = {
    {'a', "а"}, {'e', "е"}, {'o', "о"}, {'i', "и"}, {'u', "у"},
    {'b', "б"}, {'v', "в"}, {'g', "г"}, {'d', "д"}, {'z', "з"},
    {'k', "к"}, {'l', "л"}, {'m', "м"}, {'n', "н"}, {'p', "п"},
    {'r', "р"}, {'s', "с"}, {'t', "т"}, {'f', "ф"}, {'h', "х"},
    {'c', "ц"}, {'j', "й"}, {'x', "кс"},
    {'A', "А"}, {'E', "Е"}, {'O', "О"}, {'I', "И"}, {'U', "У"},
    {'B', "Б"}, {'V', "В"}, {'G', "Г"}, {'D', "Д"}, {'Z', "З"},
    {'K', "К"}, {'L', "Л"}, {'M', "М"}, {'N', "Н"}, {'P', "П"},
    {'R', "Р"}, {'S', "С"}, {'T', "Т"}, {'F', "Ф"}, {'H', "Х"},
    {'C', "Ц"}, {'J', "Й"}, {'X', "Кс"}
};

const std::map<char, char> RuleBasedAttack::cyrillicToLatin = {
    // Note: This is simplified - Cyrillic chars won't match single ASCII chars directly
    // We handle this in CyrillicToLatin function
};

RuleBasedAttack::RuleBasedAttack() : currentIndex(0) {
}

// ============= Config File Methods =============

std::string RuleBasedAttack::TrimWhitespace(const std::string& str) const {
    const size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

RuleType RuleBasedAttack::ParseRuleType(const std::string& ruleString) const {
    static const std::map<std::string, RuleType> ruleMap = {
        {"CASE_VARIATIONS", RuleType::CASE_VARIATIONS},
        {"DIGIT_SUFFIXES", RuleType::DIGIT_SUFFIXES},
        {"CYRILLIC_LAYOUT", RuleType::CYRILLIC_LAYOUT},
        {"CHARACTER_TRANSPOSE", RuleType::CHARACTER_TRANSPOSE},
        {"STRING_REVERSAL", RuleType::STRING_REVERSAL},
        {"LATIN_TO_CYRILLIC", RuleType::LATIN_TO_CYRILLIC},
        {"SPECIAL_CHARS", RuleType::SPECIAL_CHARS},
        {"YEAR_SUFFIXES", RuleType::YEAR_SUFFIXES},
        {"NAME_CITY_COMBO", RuleType::NAME_CITY_COMBO},
        {"MULTIPLE_EXCLAMATION", RuleType::MULTIPLE_EXCLAMATION},
        {"DATE_SUFFIXES", RuleType::DATE_SUFFIXES},
        {"LONG_SEQUENCES", RuleType::LONG_SEQUENCES},
        {"COMPLEX_SPECIAL_COMBOS", RuleType::COMPLEX_SPECIAL_COMBOS}
    };

    auto it = ruleMap.find(ruleString);
    return (it != ruleMap.end()) ? it->second : RuleType::INVALID;
}

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

bool RuleBasedAttack::LoadConfigFile() {
    OPENFILENAMEA ofn = {};
    char fileName[MAX_PATH] = "";

    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName);
    ofn.lpstrFilter = "Config Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Select Rule-Based Attack Configuration";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameA(&ofn)) {
        return false;
    }

    return LoadConfigFile(fileName);
}

bool RuleBasedAttack::LoadConfigFile(const std::string& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        return false;
    }

    // Clear previous state
    m_ruleSets.clear();
    variants.clear();
    variantSet.clear();
    currentIndex = 0;

    std::string line;
    size_t lineNumber = 0;

    while (std::getline(configFile, line)) {
        lineNumber++;

        const std::string trimmed = TrimWhitespace(line);

        // Skip empty lines and comments
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        // Parse: RULE_TYPE vocabulary_file_path
        size_t spacePos = trimmed.find(' ');
        if (spacePos == std::string::npos) {
            continue; // Invalid format
        }

        std::string ruleTypeString = trimmed.substr(0, spacePos);
        std::string vocabularyPath = TrimWhitespace(trimmed.substr(spacePos + 1));

        // Parse rule type
        RuleType ruleType = ParseRuleType(ruleTypeString);
        if (ruleType == RuleType::INVALID) {
            continue; // Unknown rule type
        }

        // Create and load rule set
        RuleSet ruleSet(ruleType);
        if (!LoadVocabularyFile(vocabularyPath, ruleSet.vocabulary)) {
            continue; // Failed to load vocabulary
        }

        m_ruleSets.push_back(ruleSet);
    }

    return m_ruleSets.size() > 0;
}

void RuleBasedAttack::ApplySpecificRule(RuleType rule, const std::string& word) {
    switch (rule) {
        case RuleType::CASE_VARIATIONS:
            GenerateCaseVariations(word);
            break;

        case RuleType::DIGIT_SUFFIXES:
            GenerateDigitSuffixes(word);
            break;

        case RuleType::CYRILLIC_LAYOUT: {
            AddVariant(word); // Add original
            std::string latinVersion = CyrillicToLatin(word);
            if (latinVersion != word) {
                GenerateCaseVariations(latinVersion);
                GenerateDigitSuffixes(latinVersion);
            }
            break;
        }

        case RuleType::CHARACTER_TRANSPOSE: {
            AddVariant(word); // Add original
            auto transposed = TransposeCharacters(word);
            for (const auto& trans : transposed) {
                GenerateCaseVariations(trans);
            }
            break;
        }

        case RuleType::STRING_REVERSAL: {
            AddVariant(word); // Add original
            std::string reversed = ReversePassword(word);
            GenerateCaseVariations(reversed);
            GenerateDigitSuffixes(reversed);
            break;
        }

        case RuleType::LATIN_TO_CYRILLIC: {
            AddVariant(word); // Add original
            std::string cyrillicVersion = LatinToCyrillic(word);
            if (cyrillicVersion != word) {
                GenerateCaseVariations(cyrillicVersion);
                GenerateDigitSuffixes(cyrillicVersion);
            }
            break;
        }

        case RuleType::SPECIAL_CHARS:
            GenerateSpecialCharSuffixes(word);
            break;

        case RuleType::YEAR_SUFFIXES:
            GenerateYearSuffixes(word);
            break;

        case RuleType::NAME_CITY_COMBO:
            GenerateNameCityCombos(word);
            break;

        case RuleType::MULTIPLE_EXCLAMATION:
            GenerateMultipleExclamation(word);
            break;

        case RuleType::DATE_SUFFIXES:
            GenerateDateSuffixes(word);
            break;

        case RuleType::LONG_SEQUENCES:
            GenerateLongSequences(word);
            break;

        case RuleType::COMPLEX_SPECIAL_COMBOS:
            GenerateComplexSpecialCombos(word);
            break;

        case RuleType::INVALID:
            break;
    }
}

// ============= Legacy Dictionary Methods =============

bool RuleBasedAttack::LoadDictionaryFromFile() {
    OPENFILENAMEA ofn = {};
    char fileName[MAX_PATH] = "";

    ofn.lStructSize = sizeof(OPENFILENAMEA);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName);
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = "Select Dictionary File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameA(&ofn)) {
        return false; // User cancelled
    }

    return LoadDictionaryFromFile(fileName);
}

bool RuleBasedAttack::LoadDictionaryFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    dictionary.clear();
    variants.clear();
    currentIndex = 0;

    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (!line.empty()) {
            dictionary.push_back(line);
        }
    }

    file.close();
    return true;
}

void RuleBasedAttack::GenerateVariants() {
    variants.clear();
    variantSet.clear();
    currentIndex = 0;

    // Mode 1: Config-based (each rule has its own vocabulary)
    if (m_ruleSets.size() > 0) {
        for (const auto& ruleSet : m_ruleSets) {
            for (const auto& word : ruleSet.vocabulary) {
                ApplySpecificRule(ruleSet.type, word);
            }
        }
    }
    // Mode 2: Legacy dictionary (all rules applied to all words)
    else if (dictionary.size() > 0) {
        for (const auto& basePassword : dictionary) {
            ApplyAllRules(basePassword);
        }
    }
}

std::string RuleBasedAttack::Next() {
    if (currentIndex >= variants.size()) {
        return "";
    }
    return variants[currentIndex++];
}

double RuleBasedAttack::GetProgressPercent() const {
    if (variants.empty()) return 0.0;
    return (double)currentIndex / (double)variants.size() * 100.0;
}

void RuleBasedAttack::AddVariant(const std::string& variant) {
    // Check if variant already exists using O(1) unordered_set lookup
    if (variantSet.find(variant) == variantSet.end()) {
        variantSet.insert(variant);
        variants.push_back(variant);
    }
}

void RuleBasedAttack::GenerateCaseVariations(const std::string& base) {
    AddVariant(base); // Original

    // All lowercase
    std::string lower = base;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    AddVariant(lower);

    // All uppercase
    std::string upper = base;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    AddVariant(upper);

    // Capitalize first letter only
    if (!base.empty()) {
        std::string capitalized = base;
        capitalized[0] = ::toupper(capitalized[0]);
        for (size_t i = 1; i < capitalized.length(); i++) {
            capitalized[i] = ::tolower(capitalized[i]);
        }
        AddVariant(capitalized);
    }
}

std::string RuleBasedAttack::ReversePassword(const std::string& password) {
    return std::string(password.rbegin(), password.rend());
}

void RuleBasedAttack::GenerateDigitSuffixes(const std::string& base) {
    // Add base word itself first
    AddVariant(base);
    // Add common digit suffixes
    AddVariant(base + "1");
    AddVariant(base + "12");
    AddVariant(base + "123");
    AddVariant(base + "1234");
    AddVariant(base + "2023");
    AddVariant(base + "2024");
    AddVariant(base + "2025");
    AddVariant(base + "0");
    AddVariant(base + "00");
    AddVariant(base + "123456");
    AddVariant(base + "!"); // Common special char
}

void RuleBasedAttack::GenerateSpecialCharSuffixes(const std::string& base) {
    // Add base word itself
    AddVariant(base);
    // Common special character patterns
    AddVariant(base + "!");
    AddVariant(base + "@");
    AddVariant(base + "#");
    AddVariant(base + "$");
    AddVariant(base + "%");
    AddVariant(base + "1!");
    AddVariant(base + "2@");
    AddVariant(base + "3#");
    AddVariant(base + "4$");
    AddVariant(base + "123!");
    AddVariant(base + "!@#");
    // With case variations
    if (!base.empty()) {
        std::string firstUpper = base;
        firstUpper[0] = ::toupper(firstUpper[0]);
        AddVariant(firstUpper + "1!");
        AddVariant(firstUpper + "2@");
        AddVariant(firstUpper + "3#");
        AddVariant(firstUpper + "4$");
    }
}

void RuleBasedAttack::GenerateYearSuffixes(const std::string& base) {
    // Add base word itself
    AddVariant(base);
    // Common years
    AddVariant(base + "2024");
    AddVariant(base + "2025");
    AddVariant(base + "2023");
    AddVariant(base + "2022");
    AddVariant(base + "2020");
    AddVariant(base + "2000");
    AddVariant(base + "1990");
    AddVariant(base + "1995");
    AddVariant(base + "1988");
    AddVariant(base + "2010");
    // Birth year patterns (1980s-2000s)
    for (int year = 1985; year <= 2005; year++) {
        AddVariant(base + std::to_string(year));
    }
}

void RuleBasedAttack::GenerateNameCityCombos(const std::string& base) {
    // Add base word itself
    AddVariant(base);
    
    // Common Ukrainian/Russian cities
    static const std::vector<std::string> cities = {
        "kiev", "kyiv", "lviv", "odesa", "kharkiv", 
        "dnipro", "moscow", "piter", "minsk"
    };
    
    for (const auto& city : cities) {
        AddVariant(base + city);
        // Capitalize first letter
        std::string cityCapital = city;
        if (!cityCapital.empty()) {
            cityCapital[0] = ::toupper(cityCapital[0]);
        }
        AddVariant(base + cityCapital);
        
        // Capitalize name first letter
        if (!base.empty()) {
            std::string nameCapital = base;
            nameCapital[0] = ::toupper(nameCapital[0]);
            AddVariant(nameCapital + city);
            AddVariant(nameCapital + cityCapital);
        }
    }
}

void RuleBasedAttack::GenerateMultipleExclamation(const std::string& base) {
    // Add base word itself
    AddVariant(base);
    // Multiple exclamation marks
    AddVariant(base + "!");
    AddVariant(base + "!!");
    AddVariant(base + "!!!");
    // With digits and multiple exclamation
    AddVariant(base + "1!");
    AddVariant(base + "12!");
    AddVariant(base + "123!");
    AddVariant(base + "1!!");
    AddVariant(base + "12!!");
    AddVariant(base + "123!!");
    AddVariant(base + "1!!!");
    AddVariant(base + "12!!!");
    AddVariant(base + "123!!!");
    // Long digit sequences with exclamation
    AddVariant(base + "123456!");
    AddVariant(base + "123456!!");
    AddVariant(base + "123456!!!");
    AddVariant(base + "123456789!");
    AddVariant(base + "123456789!!");
    AddVariant(base + "123456789!!!");
}

void RuleBasedAttack::GenerateDateSuffixes(const std::string& base) {
    // Add base word itself
    AddVariant(base);
    // Date formats: YYYYMMDD
    for (int year = 1985; year <= 2005; year++) {
        for (int month = 1; month <= 12; month++) {
            // Common days
            for (int day : {1, 5, 10, 15, 20, 25, 28}) {
                char dateStr[9];
                sprintf(dateStr, "%04d%02d%02d", year, month, day);
                AddVariant(base + std::string(dateStr));
            }
        }
    }
}

void RuleBasedAttack::GenerateLongSequences(const std::string& base) {
    // Add base word itself
    AddVariant(base);
    // Long alphabet sequences
    AddVariant(base + "abcdef");
    AddVariant(base + "abcdefg");
    AddVariant(base + "abcdefgh");
    AddVariant(base + "abcdefghi");
    AddVariant(base + "abcdefghij");
    AddVariant(base + "abcdefghijk");
    AddVariant(base + "abcdefghijkl");
    // Long digit sequences
    AddVariant(base + "123456");
    AddVariant(base + "1234567");
    AddVariant(base + "12345678");
    AddVariant(base + "123456789");
    // With case variations
    if (!base.empty()) {
        std::string firstUpper = base;
        firstUpper[0] = ::toupper(firstUpper[0]);
        AddVariant(firstUpper + "123456");
        AddVariant(firstUpper + "123456789");
    }
}

void RuleBasedAttack::GenerateComplexSpecialCombos(const std::string& base) {
    // Add base word itself
    AddVariant(base);
    
    // Complex patterns from passwords.txt
    // Pattern: Letter+Digit+Special (Xj8!Kv4@)
    static const std::vector<std::string> patterns = {
        "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9!"
    };
    
    // Two-part combinations
    for (size_t i = 0; i < patterns.size(); i++) {
        for (size_t j = 0; j < patterns.size(); j++) {
            if (i != j) {
                AddVariant(base + patterns[i] + patterns[j]);
            }
        }
    }
    
    // Three-part combinations (limited to common ones)
    AddVariant(base + "1!2@");
    AddVariant(base + "8!4@");
    AddVariant(base + "6#2$");
    AddVariant(base + "9!7@");
    AddVariant(base + "5#3$");
    AddVariant(base + "8!4@6#");
    AddVariant(base + "6#2$9!");
    AddVariant(base + "9!7@5#");
    AddVariant(base + "5#3$1!");
    AddVariant(base + "8@6#4!");
}

std::string RuleBasedAttack::LatinToCyrillic(const std::string& latin) {
    std::string result;
    for (char ch : latin) {
        auto it = latinToCyrillic.find(ch);
        if (it != latinToCyrillic.end()) {
            result += it->second;
        } else {
            result += ch; // Keep non-mapped characters as-is
        }
    }
    return result;
}

std::string RuleBasedAttack::CyrillicToLatin(const std::string& cyrillic) {
    // Cyrillic to Latin keyboard mapping (typing Cyrillic on English layout)
    // Maps what you would type in English to get Cyrillic letters
    static const std::map<std::string, char> cyrToLat = {
        // Lowercase Cyrillic -> Latin key position
        {"а", 'f'}, {"б", ','}, {"в", 'd'}, {"г", 'u'}, {"д", 'l'},
        {"е", 't'}, {"ё", '`'}, {"ж", ';'}, {"з", 'p'}, {"и", 'b'},
        {"й", 'q'}, {"к", 'r'}, {"л", 'k'}, {"м", 'v'}, {"н", 'y'},
        {"о", 'j'}, {"п", 'g'}, {"р", 'h'}, {"с", 'c'}, {"т", 'n'},
        {"у", 'e'}, {"ф", 'a'}, {"х", '['}, {"ц", 'w'}, {"ч", 'x'},
        {"ш", 'i'}, {"щ", 'o'}, {"ъ", ']'}, {"ы", 's'}, {"ь", 'm'},
        {"э", '\''}, {"ю", '.'}, {"я", 'z'},
        // Uppercase Cyrillic -> Latin key position (with Shift)
        {"А", 'F'}, {"Б", '<'}, {"В", 'D'}, {"Г", 'U'}, {"Д", 'L'},
        {"Е", 'T'}, {"Ё", '~'}, {"Ж", ':'}, {"З", 'P'}, {"И", 'B'},
        {"Й", 'Q'}, {"К", 'R'}, {"Л", 'K'}, {"М", 'V'}, {"Н", 'Y'},
        {"О", 'J'}, {"П", 'G'}, {"Р", 'H'}, {"С", 'C'}, {"Т", 'N'},
        {"У", 'E'}, {"Ф", 'A'}, {"Х", '{'}, {"Ц", 'W'}, {"Ч", 'X'},
        {"Ш", 'I'}, {"Щ", 'O'}, {"Ъ", '}'}, {"Ы", 'S'}, {"Ь", 'M'},
        {"Э", '"'}, {"Ю", '>'}, {"Я", 'Z'}
    };

    std::string result;
    size_t i = 0;
    
    while (i < cyrillic.length()) {
        bool found = false;
        
        // Try to match 2-byte UTF-8 Cyrillic character
        if (i + 1 < cyrillic.length()) {
            std::string twoBytes = cyrillic.substr(i, 2);
            auto it = cyrToLat.find(twoBytes);
            if (it != cyrToLat.end()) {
                result += it->second;
                i += 2;
                found = true;
            }
        }
        
        if (!found) {
            // Keep non-Cyrillic characters as-is
            result += cyrillic[i];
            i++;
        }
    }
    
    return result;
}

// Character transposition rule (requirements.md Section 3.B.3)
std::vector<std::string> RuleBasedAttack::TransposeCharacters(const std::string& password) {
    std::vector<std::string> transposed;
    
    if (password.length() < 2) {
        return transposed;
    }

    // Generate all adjacent character swaps
    for (size_t i = 0; i < password.length() - 1; i++) {
        std::string variant = password;
        std::swap(variant[i], variant[i + 1]);
        transposed.push_back(variant);
    }

    return transposed;
}

void RuleBasedAttack::ApplyAllRules(const std::string& base) {
    // Rule 1: Original and case variations
    GenerateCaseVariations(base);

    // Rule 2: Reversed
    std::string reversed = ReversePassword(base);
    GenerateCaseVariations(reversed);

    // Rule 3: With digit suffixes
    GenerateDigitSuffixes(base);

    // Rule 4: Latin to Cyrillic conversion (typing "ghbdtn" instead of "привет")
    std::string cyrillic = LatinToCyrillic(base);
    if (cyrillic != base) {
        GenerateCaseVariations(cyrillic);
        GenerateDigitSuffixes(cyrillic);
    }

    // Rule 5: Reversed with digits
    GenerateDigitSuffixes(reversed);

    // Rule 6: Cyrillic reversed
    std::string cyrillicReversed = ReversePassword(cyrillic);
    if (cyrillicReversed != base && cyrillicReversed != cyrillic) {
        GenerateCaseVariations(cyrillicReversed);
    }
    
    // Rule 7: Cyrillic to Latin conversion (requirements.md Section 3.B.3)
    std::string latin = CyrillicToLatin(base);
    if (latin != base) {
        GenerateCaseVariations(latin);
        GenerateDigitSuffixes(latin);
    }
    
    // Rule 8: Character transposition (requirements.md Section 3.B.3)
    auto transpositions = TransposeCharacters(base);
    for (const auto& trans : transpositions) {
        AddVariant(trans);
        // Also add case variations for transposed passwords
        std::string lower = trans;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        AddVariant(lower);
        std::string upper = trans;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        AddVariant(upper);
    }
}
