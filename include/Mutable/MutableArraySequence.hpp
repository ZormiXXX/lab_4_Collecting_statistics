#pragma once
#include "../ArraySequence.hpp"

template<class T>
class MutableArraySequence : public ArraySequence<T> {
protected:
    MutableArraySequence<T>* Clone() const override {
        return new MutableArraySequence<T>(*this);
    }

    MutableArraySequence<T>* Instance(int expectedLength) override {
        this->items->Reserve(expectedLength);
        return this;
    }

public:
    MutableArraySequence() : ArraySequence<T>() {}

    MutableArraySequence(const T* arr, int count) : ArraySequence<T>(arr, count) {}

    explicit MutableArraySequence(DynamicArray<T>* arr) : ArraySequence<T>(arr) {}

    MutableArraySequence(const MutableArraySequence<T>& other)
        : ArraySequence<T>(new DynamicArray<T>(*other.items)) {}

    explicit MutableArraySequence(const LinkedList<T>& list) : ArraySequence<T>(list) {}

    explicit MutableArraySequence(const LinkedList<T>* list) : ArraySequence<T>(list) {}

    ~MutableArraySequence() override = default;

    Sequence<T>* CreateEmpty() const override {
        return new MutableArraySequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* items, int count) const override {
        return new MutableArraySequence<T>(items, count);
    }
};
