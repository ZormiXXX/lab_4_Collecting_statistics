#pragma once
#include "Exceptions.hpp"

template<class T>
class Option {
private:
    T* value;

    void Clear() {
        delete value;
        value = nullptr;
    }

public:
    Option() : value(nullptr) {}

    Option(const Option<T>& other) : value(nullptr) {
        if (other.value != nullptr) {
            value = new T(*other.value);
        }
    }

    Option(Option<T>&& other) noexcept : value(other.value) {
        other.value = nullptr;
    }

    ~Option() {
        Clear();
    }

    Option<T>& operator=(const Option<T>& other) {
        if (this != &other) {
            Clear();
            if (other.value != nullptr) {
                value = new T(*other.value);
            }
        }
        return *this;
    }

    Option<T>& operator=(Option<T>&& other) noexcept {
        if (this != &other) {
            Clear();
            value = other.value;
            other.value = nullptr;
        }
        return *this;
    }

    static Option<T> None() {
        return Option<T>();
    }

    static Option<T> Some(const T& val) {
        Option<T> opt;
        opt.value = new T(val);
        return opt;
    }

    bool IsSome() const {
        return value != nullptr;
    }

    bool IsNone() const {
        return value == nullptr;
    }

    const T& GetValue() const {
        if (value == nullptr) {
            throw NoValuePresent();
        }
        return *value;
    }

    T GetValueOrDefault(const T& defaultVal) const {
        if (value == nullptr) {
            return defaultVal;
        }
        return *value;
    }
};
