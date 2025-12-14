#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include "LinkedList.h"
#include "PasswordGenerator.h"

// Rule type enumeration (type-safe)
enum class RuleType {
    INVALID = 0,
    CASE_VARIATIONS,
    DIGIT_SUFFIXES,
    CYRILLIC_LAYOUT,
    CHARACTER_TRANSPOSE,
    STRING_REVERSAL,
    LATIN_TO_CYRILLIC,
    SPECIAL_CHARS,
    YEAR_SUFFIXES,
    NAME_CITY_COMBO,
    MULTIPLE_EXCLAMATION,
    DATE_SUFFIXES,
    LONG_SEQUENCES,
    COMPLEX_SPECIAL_COMBOS
};

// Rule set: links a rule type to its vocabulary (singly linked list)
struct RuleSet {
    RuleType type;
    LinkedList<std::string> vocabulary;

    explicit RuleSet(RuleType ruleType = RuleType::INVALID) : type(ruleType) {}
};

class RuleBasedAttack : public PasswordGenerator {
private:
    // Config-based storage: list of rule sets (each with its own vocabulary)
    LinkedList<RuleSet> m_ruleSets;
    
    // Legacy single dictionary support
    LinkedList<std::string> dictionary;
    
    // Generated password variants
    LinkedList<std::string> variants;
    std::unordered_set<std::string> variantSet; // For O(1) deduplication
    size_t currentIndex;

    static const std::map<char, std::string> latinToCyrillic;
    static const std::map<char, char> cyrillicToLatin;

public:
    RuleBasedAttack();
    
    // Config file loading (new implementation)
    bool LoadConfigFile();
    bool LoadConfigFile(const std::string& configPath);
    
    // Legacy dictionary loading (backward compatibility)
    bool LoadDictionaryFromFile();
    bool LoadDictionaryFromFile(const std::string& filename);
    
    void GenerateVariants();
    
    // PasswordGenerator interface implementation
    std::string Next() override;
    void Reset() override { currentIndex = 0; }
    bool HasNext() const override { return currentIndex < variants.size(); }
    double GetProgressPercent() const override;
    size_t GetTotalCount() const override { return variants.size(); }
    
    // Rule-based attack specific methods
    size_t GetVariantCount() const { return variants.size(); }
    size_t GetDictionarySize() const { return dictionary.size(); }
    size_t GetRuleSetCount() const { return m_ruleSets.size(); }
    std::string GetVariantAtIndex(size_t index) const { return variants[index]; }

private:
    // Helper methods
    void AddVariant(const std::string& variant);
    std::string TrimWhitespace(const std::string& str) const;
    RuleType ParseRuleType(const std::string& ruleString) const;
    bool LoadVocabularyFile(const std::string& filename, LinkedList<std::string>& vocabulary) const;
    
    // Rule application methods
    void ApplySpecificRule(RuleType rule, const std::string& word);
    void GenerateCaseVariations(const std::string& base);
    std::string ReversePassword(const std::string& password);
    void GenerateDigitSuffixes(const std::string& base);
    void GenerateSpecialCharSuffixes(const std::string& base);
    void GenerateYearSuffixes(const std::string& base);
    void GenerateNameCityCombos(const std::string& base);
    void GenerateMultipleExclamation(const std::string& base);
    void GenerateDateSuffixes(const std::string& base);
    void GenerateLongSequences(const std::string& base);
    void GenerateComplexSpecialCombos(const std::string& base);
    std::string LatinToCyrillic(const std::string& latin);
    std::string CyrillicToLatin(const std::string& cyrillic);
    std::vector<std::string> TransposeCharacters(const std::string& password);
    void ApplyAllRules(const std::string& base);
};
