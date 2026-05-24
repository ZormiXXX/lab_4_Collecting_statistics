#pragma once
#include "LinkedList.hpp"
#include "Sequence.hpp"

template<class T>
class ImmutableListSequence;

template<class T>
class ListSequence : public SequenceCRTP<T, ListSequence<T>> {
    template<class, class>
    friend class SequenceCRTP;

    template<class>
    friend class ImmutableListSequence;

protected:
    LinkedList<T>* items;

    virtual ListSequence<T>* Clone() const {
        return new ListSequence<T>(*this);
    }

    virtual ListSequence<T>* Instance(int expectedLength) {
        (void)expectedLength;
        return this;
    }

    void AppendInternal(const T& item) {
        items->Append(item);
    }

    void PrependInternal(const T& item) {
        items->Prepend(item);
    }

    void InsertAtInternal(const T& item, int index) {
        items->InsertAt(item, index);
    }

    void ConcatInternal(const Sequence<T>* other) {
        int limit = other == this ? GetLength() : other->GetLength();
        IEnumerator<T>* enumerator = other->GetEnumerator();
        int copied = 0;
        while (copied < limit && enumerator->MoveNext()) {
            items->Append(enumerator->GetCurrent());
            copied++;
        }
        delete enumerator;
    }

public:
    ListSequence() : items(new LinkedList<T>()) {}

    ListSequence(const T* arr, int count) : items(new LinkedList<T>(arr, count)) {}

    ListSequence(const ListSequence<T>& other) : items(new LinkedList<T>(*other.items)) {}

    explicit ListSequence(const LinkedList<T>& list) : items(new LinkedList<T>(list)) {}

    explicit ListSequence(const LinkedList<T>* list) : items(new LinkedList<T>(*list)) {}

    explicit ListSequence(LinkedList<T>* list) : items(list) {}

    ~ListSequence() override {
        delete items;
    }

    LinkedList<T>* CopyStorage() const {
        return new LinkedList<T>(*items);
    }

    LinkedList<T>* ReleaseStorage() {
        LinkedList<T>* released = items;
        items = nullptr;
        return released;
    }

    const T& Get(int index) const override {
        return items->Get(index);
    }

    int GetLength() const override {
        return items->GetLength();
    }

    const T& GetFirst() const override {
        return items->GetFirst();
    }

    const T& GetLast() const override {
        return items->GetLast();
    }

    Sequence<T>* CreateEmpty() const override {
        return new ListSequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* itemsArr, int count) const override {
        return new ListSequence<T>(itemsArr, count);
    }

    IEnumerator<T>* GetEnumerator() const override {
        return items->GetEnumerator();
    }

    Sequence<T>* GetSubsequence(int start, int end) const override {
        if (start < 0 || end >= GetLength() || start > end) {
            throw IndexOutOfRange(start, GetLength());
        }

        Sequence<T>* result = this->CreateAccumulator(end - start + 1);
        IEnumerator<T>* enumerator = items->GetEnumerator();
        int index = 0;
        while (enumerator->MoveNext()) {
            if (index > end) {
                break;
            }
            if (index >= start) {
                Sequence<T>* updated = result->Append(enumerator->GetCurrent());
                if (updated != result) {
                    delete result;
                }
                result = updated;
            }
            index++;
        }
        delete enumerator;
        return this->FinalizeAccumulator(result);
    }
};
