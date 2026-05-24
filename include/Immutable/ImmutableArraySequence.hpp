#pragma once
#include "../ArraySequence.hpp"

template<class T>
class ImmutableArraySequence : public ArraySequence<T> {
protected:
    ImmutableArraySequence<T>* Clone() const override {
        return new ImmutableArraySequence<T>(*this);
    }

    Sequence<T>* CreateAccumulator() const override {
        return new ArraySequence<T>();
    }

    Sequence<T>* CreateAccumulator(int expectedLength) const override {
        return new ArraySequence<T>(expectedLength);
    }

    Sequence<T>* FinalizeAccumulator(Sequence<T>* accumulator) const override {
        auto* builder = static_cast<ArraySequence<T>*>(accumulator);
        Sequence<T>* result = new ImmutableArraySequence<T>(builder->ReleaseStorage());
        delete accumulator;
        return result;
    }

    ImmutableArraySequence<T>* Instance(int expectedLength) override {
        return new ImmutableArraySequence<T>(this->CopyStorage(expectedLength));
    }

public:
    ImmutableArraySequence() : ArraySequence<T>() {}

    ImmutableArraySequence(const T* arr, int count) : ArraySequence<T>(arr, count) {}

    explicit ImmutableArraySequence(DynamicArray<T>* arr) : ArraySequence<T>(arr) {}

    ImmutableArraySequence(const ImmutableArraySequence<T>& other)
        : ArraySequence<T>(new DynamicArray<T>(*other.items)) {}

    explicit ImmutableArraySequence(const LinkedList<T>& list) : ArraySequence<T>(list) {}

    explicit ImmutableArraySequence(const LinkedList<T>* list) : ArraySequence<T>(list) {}

    ~ImmutableArraySequence() override = default;

    Sequence<T>* CreateEmpty() const override {
        return new ImmutableArraySequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* items, int count) const override {
        return new ImmutableArraySequence<T>(items, count);
    }
};
