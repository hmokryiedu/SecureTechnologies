#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_set>

class RuleBasedAttack {
private:
    std::vector<std::string> dictionary;
    std::vector<std::string> variants;
    std::unordered_set<std::string> variantSet;
    size_t currentIndex;

    static const std::map<char, std::string> latinToCyrillic;
    static const std::map<char, char> cyrillicToLatin;

public:
    RuleBasedAttack();
    bool LoadDictionaryFromFile();
    bool LoadDictionaryFromFile(const std::string& filename);
    void GenerateVariants();
    std::string Next();
    void Reset() { currentIndex = 0; }
    bool HasNext() const { return currentIndex < variants.size(); }
    size_t GetVariantCount() const { return variants.size(); }
    size_t GetDictionarySize() const { return dictionary.size(); }
    double GetProgressPercent() const;

private:
    void AddVariant(const std::string& variant);
    void GenerateCaseVariations(const std::string& base);
    std::string ReversePassword(const std::string& password);
    void GenerateDigitSuffixes(const std::string& base);
    std::string LatinToCyrillic(const std::string& latin);
    std::string CyrillicToLatin(const std::string& cyrillic);
    void ApplyAllRules(const std::string& base);
};
