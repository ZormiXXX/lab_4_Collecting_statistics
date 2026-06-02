#pragma once

inline Cardinal::Cardinal() : value(0), infinite(false) {}

inline Cardinal::Cardinal(size_t finiteValue) : value(finiteValue), infinite(false) {}

inline Cardinal Cardinal::Finite(size_t finiteValue) {
    return Cardinal(finiteValue);
}

inline Cardinal Cardinal::Infinite() {
    Cardinal cardinal;
    cardinal.infinite = true;
    cardinal.value = 0;
    return cardinal;
}

inline bool Cardinal::IsInfinite() const {
    return infinite;
}

inline bool Cardinal::IsFinite() const {
    return !infinite;
}

inline size_t Cardinal::AsFinite() const {
    if (infinite) {
        throw InfinityError("cardinality is infinite");
    }
    return value;
}

inline std::string Cardinal::ToString() const {
    return infinite ? "omega" : std::to_string(value);
}

inline Cardinal Cardinal::Add(size_t delta) const {
    if (infinite) {
        return Infinite();
    }
    return Finite(value + delta);
}

inline Cardinal Cardinal::Add(const Cardinal& left, const Cardinal& right) {
    if (left.IsInfinite() || right.IsInfinite()) {
        return Infinite();
    }
    return Finite(left.value + right.value);
}

inline Cardinal Cardinal::Min(const Cardinal& left, const Cardinal& right) {
    if (left.IsInfinite()) {
        return right;
    }
    if (right.IsInfinite()) {
        return left;
    }
    return Finite(left.value < right.value ? left.value : right.value);
}

inline Cardinal Cardinal::Max(const Cardinal& left, const Cardinal& right) {
    if (left.IsInfinite() || right.IsInfinite()) {
        return Infinite();
    }
    return Finite(left.value > right.value ? left.value : right.value);
}

inline bool Cardinal::operator==(const Cardinal& other) const {
    if (infinite != other.infinite) {
        return false;
    }
    return infinite || value == other.value;
}

inline bool Cardinal::operator!=(const Cardinal& other) const {
    return !(*this == other);
}

inline Ordinal::Ordinal() : offset(0), hasOmega(false) {}

inline Ordinal::Ordinal(size_t finiteValue) : offset(finiteValue), hasOmega(false) {}

inline Ordinal Ordinal::Finite(size_t finiteValue) {
    return Ordinal(finiteValue);
}

inline Ordinal Ordinal::Omega() {
    Ordinal ordinal;
    ordinal.hasOmega = true;
    ordinal.offset = 0;
    return ordinal;
}

inline Ordinal Ordinal::OmegaPlus(size_t finiteOffset) {
    Ordinal ordinal;
    ordinal.hasOmega = true;
    ordinal.offset = finiteOffset;
    return ordinal;
}

inline bool Ordinal::IsFinite() const {
    return !hasOmega;
}

inline bool Ordinal::HasOmega() const {
    return hasOmega;
}

inline bool Ordinal::IsPureOmega() const {
    return hasOmega && offset == 0;
}

inline size_t Ordinal::AsFinite() const {
    if (hasOmega) {
        throw InfinityError("ordinal contains omega");
    }
    return offset;
}

inline size_t Ordinal::GetOffset() const {
    return offset;
}

inline Ordinal Ordinal::Add(size_t delta) const {
    return hasOmega ? OmegaPlus(offset + delta) : Finite(offset + delta);
}

inline std::string Ordinal::ToString() const {
    if (!hasOmega) {
        return std::to_string(offset);
    }
    if (offset == 0) {
        return "omega";
    }
    return "omega + " + std::to_string(offset);
}

inline bool Ordinal::operator==(const Ordinal& other) const {
    return hasOmega == other.hasOmega && offset == other.offset;
}

inline bool Ordinal::operator!=(const Ordinal& other) const {
    return !(*this == other);
}
