#pragma once
#include "../ListSequence.hpp"

template<class T>
class MutableListSequence : public ListSequence<T> {
protected:
    MutableListSequence<T>* Clone() const override {
        return new MutableListSequence<T>(*this);
    }

    MutableListSequence<T>* Instance(int expectedLength) override {
        (void)expectedLength;
        return this;
    }

    Sequence<T>* CreateAccumulator(int expectedLength) const override {
        (void)expectedLength;
        return new MutableListSequence<T>();
    }

public:
    MutableListSequence() : ListSequence<T>() {}

    MutableListSequence(const T* arr, int count) : ListSequence<T>(arr, count) {}

    MutableListSequence(const MutableListSequence<T>& other)
        : ListSequence<T>(new LinkedList<T>(*other.items)) {}

    explicit MutableListSequence(const LinkedList<T>& list) : ListSequence<T>(list) {}

    explicit MutableListSequence(const LinkedList<T>* list) : ListSequence<T>(list) {}

    ~MutableListSequence() override = default;

    Sequence<T>* CreateEmpty() const override {
        return new MutableListSequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* items, int count) const override {
        return new MutableListSequence<T>(items, count);
    }
};
