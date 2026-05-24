#pragma once
#include <ostream>
#include <string>
#include "Exceptions.hpp"

class Cardinal {
private:
    size_t value;
    bool infinite;

public:
    Cardinal() : value(0), infinite(false) {}

    explicit Cardinal(size_t finiteValue) : value(finiteValue), infinite(false) {}

    static Cardinal Finite(size_t finiteValue) {
        return Cardinal(finiteValue);
    }

    static Cardinal Infinite() {
        Cardinal cardinal;
        cardinal.infinite = true;
        cardinal.value = 0;
        return cardinal;
    }

    bool IsInfinite() const {
        return infinite;
    }

    bool IsFinite() const {
        return !infinite;
    }

    size_t AsFinite() const {
        if (infinite) {
            throw InfinityError("cardinality is infinite");
        }
        return value;
    }

    std::string ToString() const {
        return infinite ? "omega" : std::to_string(value);
    }

    Cardinal Add(size_t delta) const {
        if (infinite) {
            return Infinite();
        }
        return Finite(value + delta);
    }

    static Cardinal Add(const Cardinal& left, const Cardinal& right) {
        if (left.IsInfinite() || right.IsInfinite()) {
            return Infinite();
        }
        return Finite(left.value + right.value);
    }

    static Cardinal Min(const Cardinal& left, const Cardinal& right) {
        if (left.IsInfinite()) {
            return right;
        }
        if (right.IsInfinite()) {
            return left;
        }
        return Finite(left.value < right.value ? left.value : right.value);
    }

    static Cardinal Max(const Cardinal& left, const Cardinal& right) {
        if (left.IsInfinite() || right.IsInfinite()) {
            return Infinite();
        }
        return Finite(left.value > right.value ? left.value : right.value);
    }

    bool operator==(const Cardinal& other) const {
        if (infinite != other.infinite) {
            return false;
        }
        return infinite || value == other.value;
    }

    bool operator!=(const Cardinal& other) const {
        return !(*this == other);
    }
};

inline std::ostream& operator<<(std::ostream& stream, const Cardinal& cardinal) {
    stream << cardinal.ToString();
    return stream;
}
