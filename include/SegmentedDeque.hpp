#pragma once

#include "ArraySequence.hpp"
#include "DynamicArray.hpp"
#include "Exceptions.hpp"
#include "ListSequence.hpp"
#include "Sequence.hpp"
#include <functional>
#include <string>

enum class DequeStorageKind {
    ArraySequence,
    ListSequence
};

inline std::string DequeStorageKindToString(DequeStorageKind kind) {
    return kind == DequeStorageKind::ArraySequence ? "ArraySequence" : "ListSequence";
}

template<class T>
class SegmentedDeque {
protected:
    struct Block {
        DynamicArray<T> data;
        int begin;
        int end;

        Block(int capacity, int startIndex);
        Block(const Block& other);

        int Capacity() const;
        int Size() const;
        bool Empty() const;
        bool CanPushFront() const;
        bool CanPushBack() const;

        void PushFront(const T& item);
        void PushBack(const T& item);
        void PopFront();
        void PopBack();
        const T& Get(int index) const;
    };

    template<class Pointer>
    static void ReplaceOwned(Pointer*& current, Pointer* updated);

    template<class U>
    static void AppendOwned(Sequence<U>*& sequence, const U& value);

    static void AppendDequeTo(SegmentedDeque<T>& target, const SegmentedDeque<T>& source);
    static void AppendSequenceTo(SegmentedDeque<T>& target, const Sequence<T>& source);

    Sequence<Block*>* blocks;
    int blockCapacity;
    int totalSize;
    DequeStorageKind storageKind;

    virtual SegmentedDeque<T>* Instance() = 0;
    virtual SegmentedDeque<T>* CreateEmptySameKind() const = 0;

    template<class U>
    Sequence<U>* CreateSequenceForKind() const;

    Sequence<Block*>* CreateBlockSequence(DequeStorageKind kind) const;
    void DestroyBlocks();
    Block* MakeBlock(int startIndex) const;
    Block* EnsureFrontBlockForPush();
    Block* EnsureBackBlockForPush();
    void RemoveBoundaryBlock(bool fromFront);
    const Block& FindBlock(int index, int& localIndex) const;
    const T& GetRef(int index) const;

    template<class Visitor>
    void ForEachElement(Visitor visitor) const;

    template<class Visitor>
    void ForEachBlock(Visitor visitor) const;

    SegmentedDeque<T>* BuildFromSequence(const Sequence<T>& source) const;

    template<class Transformer>
    SegmentedDeque<T>* BuildFromTransformedSequence(Transformer transform) const;

    void PushBackDirect(const T& item);
    void PushFrontDirect(const T& item);
    void PopFrontDirect();
    void PopBackDirect();

public:
    explicit SegmentedDeque(
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    );

    SegmentedDeque(
        const T* items,
        int count,
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    );

    SegmentedDeque(const SegmentedDeque<T>& other);
    SegmentedDeque<T>& operator=(const SegmentedDeque<T>&) = delete;
    virtual ~SegmentedDeque();

    int GetLength() const;
    int GetBlockCapacity() const;
    int GetSegmentCount() const;
    DequeStorageKind GetStorageKind() const;
    bool IsEmpty() const;

    T Get(int index) const;
    T Front() const;
    T Back() const;

    SegmentedDeque<T>* PushFront(const T& item);
    SegmentedDeque<T>* PushBack(const T& item);
    SegmentedDeque<T>* PopFront();
    SegmentedDeque<T>* PopBack();

    SegmentedDeque<T>* Concat(const SegmentedDeque<T>& other) const;
    SegmentedDeque<T>* GetSubDeque(int start, int end) const;
    int FindSubDeque(const SegmentedDeque<T>& pattern) const;
    bool ContainsSubDeque(const SegmentedDeque<T>& pattern) const;

    SegmentedDeque<T>* Map(const std::function<T(const T&)>& mapper) const;
    SegmentedDeque<T>* Where(const std::function<bool(const T&)>& predicate) const;
    T Reduce(const std::function<T(const T&, const T&)>& reducer, const T& initial) const;

    SegmentedDeque<T>* Sorted(
        const std::function<bool(const T&, const T&)>& comparator = std::less<T>()
    ) const;

    SegmentedDeque<T>* MergeSorted(
        const SegmentedDeque<T>& other,
        const std::function<bool(const T&, const T&)>& comparator = std::less<T>()
    ) const;

    Sequence<T>* ToSequence() const;
    std::string DescribeLayout() const; // убрать логику отображения

    T operator[](int index) const {
        return Get(index);
    }
};

#include "SegmentedDeque.tpp"
