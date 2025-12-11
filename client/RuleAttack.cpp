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

    // Apply all transformation rules to each dictionary entry
    // AddVariant() handles duplicate checking via variantSet
    for (const auto& basePassword : dictionary) {
        ApplyAllRules(basePassword);
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
    // Add common digit suffixes
    AddVariant(base + "1");
    AddVariant(base + "12");
    AddVariant(base + "123");
    AddVariant(base + "1234");
    AddVariant(base + "2023");
    AddVariant(base + "2024");
    AddVariant(base + "0");
    AddVariant(base + "00");
    AddVariant(base + "123456");
    AddVariant(base + "!"); // Common special char
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
