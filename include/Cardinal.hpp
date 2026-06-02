#pragma once
#include <string>
#include "Exceptions.hpp"

class Cardinal {
private:
    size_t value;
    bool infinite;

public:
    Cardinal();
    explicit Cardinal(size_t finiteValue);

    static Cardinal Finite(size_t finiteValue);
    static Cardinal Infinite();

    bool IsInfinite() const;
    bool IsFinite() const;
    size_t AsFinite() const;
    std::string ToString() const;
    Cardinal Add(size_t delta) const;

    static Cardinal Add(const Cardinal& left, const Cardinal& right);
    static Cardinal Min(const Cardinal& left, const Cardinal& right);
    static Cardinal Max(const Cardinal& left, const Cardinal& right);

    bool operator==(const Cardinal& other) const;
    bool operator!=(const Cardinal& other) const;
};

class Ordinal {
private:
    size_t offset;
    bool hasOmega;

public:
    Ordinal();
    explicit Ordinal(size_t finiteValue);

    static Ordinal Finite(size_t finiteValue);
    static Ordinal Omega();
    static Ordinal OmegaPlus(size_t finiteOffset);

    bool IsFinite() const;
    bool HasOmega() const;
    bool IsPureOmega() const;
    size_t AsFinite() const;
    size_t GetOffset() const;
    Ordinal Add(size_t delta) const;
    std::string ToString() const;

    bool operator==(const Ordinal& other) const;
    bool operator!=(const Ordinal& other) const;
};

#include "Cardinal.tpp"
