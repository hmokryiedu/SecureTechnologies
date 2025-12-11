#pragma once

#include <string>
#include <cstddef>

// Abstract base class for password generators (OOP polymorphism - requirements.md Section 2)
class PasswordGenerator {
public:
    virtual ~PasswordGenerator() = default;
    
    // Pure virtual methods (interface)
    virtual bool HasNext() const = 0;
    virtual std::string Next() = 0;
    virtual void Reset() = 0;
    virtual double GetProgressPercent() const = 0;
    virtual size_t GetTotalCount() const = 0;
};
