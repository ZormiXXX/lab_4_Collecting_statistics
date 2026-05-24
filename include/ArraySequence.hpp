#pragma once
#include "DynamicArray.hpp"
#include "LinkedList.hpp"
#include "Sequence.hpp"

template<class T>
class ImmutableArraySequence;

template<class T>
class ArraySequence : public SequenceCRTP<T, ArraySequence<T>> {
    template<class, class>
    friend class SequenceCRTP;

    template<class>
    friend class ImmutableArraySequence;

protected:
    DynamicArray<T>* items;

    virtual ArraySequence<T>* Clone() const {
        return new ArraySequence<T>(*this);
    }

    virtual ArraySequence<T>* Instance(int expectedLength) {
        (void)expectedLength;
        return this;
    }

    void AppendInternal(const T& item) {
        items->Append(item);
    }

    void PrependInternal(const T& item) {
        items->InsertAt(0, item);
    }

    void InsertAtInternal(const T& item, int index) {
        if (index < 0 || index >= items->GetSize()) {
            throw IndexOutOfRange(index, items->GetSize());
        }
        items->InsertAt(index, item);
    }

    void ConcatInternal(const Sequence<T>* other) {
        int originalLength = GetLength();
        int otherLength = other->GetLength();
        items->Reserve(originalLength + otherLength);

        if (other == this) {
            for (int i = 0; i < originalLength; i++) {
                AppendInternal(items->Get(i));
            }
            return;
        }

        IEnumerator<T>* enumerator = other->GetEnumerator();
        while (enumerator->MoveNext()) {
            AppendInternal(enumerator->GetCurrent());
        }
        delete enumerator;
    }

public:
    ArraySequence() : items(new DynamicArray<T>()) {}

    explicit ArraySequence(int expectedLength) : items(new DynamicArray<T>()) {
        items->Reserve(expectedLength);
    }

    ArraySequence(const T* arr, int count) : items(new DynamicArray<T>(arr, count)) {}

    ArraySequence(DynamicArray<T>* arr) : items(arr) {}

    ArraySequence(const ArraySequence<T>& other) : items(new DynamicArray<T>(*other.items)) {}

    ArraySequence(const LinkedList<T>& list) : items(new DynamicArray<T>(list.GetLength())) {
        list.CopyToArray(items->RawData());
    }

    ArraySequence(const LinkedList<T>* list) : ArraySequence(*list) {}

    ~ArraySequence() override {
        delete items;
    }

    DynamicArray<T>* CopyStorage() const {
        return new DynamicArray<T>(*items);
    }

    DynamicArray<T>* CopyStorage(int minCapacity) const {
        return new DynamicArray<T>(*items, minCapacity);
    }

    DynamicArray<T>* ReleaseStorage() {
        DynamicArray<T>* released = items;
        items = nullptr;
        return released;
    }

    const T& Get(int index) const override {
        return items->Get(index);
    }

    int GetLength() const override {
        return items->GetSize();
    }

    const T& GetFirst() const override {
        return items->Get(0);
    }

    const T& GetLast() const override {
        if (GetLength() == 0) {
            throw IndexOutOfRange(0, GetLength());
        }
        return items->Get(GetLength() - 1);
    }

    Sequence<T>* CreateEmpty() const override {
        return new ArraySequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* source, int count) const override {
        return new ArraySequence<T>(source, count);
    }

    IEnumerator<T>* GetEnumerator() const override {
        return items->GetEnumerator();
    }

    Sequence<T>* GetSubsequence(int start, int end) const override {
        if (start < 0 || end >= GetLength() || start > end) {
            throw IndexOutOfRange(start, GetLength());
        }

        Sequence<T>* result = this->CreateAccumulator(end - start + 1);
        for (int i = start; i <= end; i++) {
            Sequence<T>* updated = result->Append(items->Get(i));
            if (updated != result) {
                delete result;
            }
            result = updated;
        }
        return this->FinalizeAccumulator(result);
    }

    T& operator[](int index) {
        return (*items)[index];
    }

    const T& operator[](int index) const {
        return (*items)[index];
    }
};
