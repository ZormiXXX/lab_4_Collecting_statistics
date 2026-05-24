#pragma once
#include "../ListSequence.hpp"

template<class T>
class ImmutableListSequence : public ListSequence<T> {
protected:
    ImmutableListSequence<T>* Clone() const override {
        return new ImmutableListSequence<T>(*this);
    }

    Sequence<T>* CreateAccumulator() const override {
        return new ListSequence<T>();
    }

    Sequence<T>* CreateAccumulator(int expectedLength) const override {
        (void)expectedLength;
        return new ListSequence<T>();
    }

    Sequence<T>* FinalizeAccumulator(Sequence<T>* accumulator) const override {
        auto* builder = static_cast<ListSequence<T>*>(accumulator);
        Sequence<T>* result = new ImmutableListSequence<T>(builder->ReleaseStorage());
        delete accumulator;
        return result;
    }

    ImmutableListSequence<T>* Instance(int expectedLength) override {
        (void)expectedLength;
        return Clone();
    }

public:
    ImmutableListSequence() : ListSequence<T>() {}

    ImmutableListSequence(const T* arr, int count) : ListSequence<T>(arr, count) {}

    ImmutableListSequence(const ImmutableListSequence<T>& other)
        : ListSequence<T>(new LinkedList<T>(*other.items)) {}

    explicit ImmutableListSequence(const LinkedList<T>& list) : ListSequence<T>(list) {}

    explicit ImmutableListSequence(const LinkedList<T>* list) : ListSequence<T>(list) {}

    explicit ImmutableListSequence(LinkedList<T>* list) : ListSequence<T>(list) {}

    ~ImmutableListSequence() override = default;

    Sequence<T>* CreateEmpty() const override {
        return new ImmutableListSequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* items, int count) const override {
        return new ImmutableListSequence<T>(items, count);
    }
};
