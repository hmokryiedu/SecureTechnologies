#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include "LinkedList.h"
#include "PasswordGenerator.h"

class RuleBasedAttack : public PasswordGenerator {
private:
    LinkedList<std::string> dictionary;   // Templated linked list (requirements.md Section 3.B.3)
    LinkedList<std::string> variants;     // Templated linked list for password variants
    std::unordered_set<std::string> variantSet; // For O(1) deduplication
    size_t currentIndex;

    static const std::map<char, std::string> latinToCyrillic;
    static const std::map<char, char> cyrillicToLatin;

public:
    RuleBasedAttack();
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
    std::string GetVariantAtIndex(size_t index) const { return variants[index]; }

private:
    void AddVariant(const std::string& variant);
    void GenerateCaseVariations(const std::string& base);
    std::string ReversePassword(const std::string& password);
    void GenerateDigitSuffixes(const std::string& base);
    std::string LatinToCyrillic(const std::string& latin);
    std::string CyrillicToLatin(const std::string& cyrillic);
    std::vector<std::string> TransposeCharacters(const std::string& password);
    void ApplyAllRules(const std::string& base);
};
