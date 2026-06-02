#pragma once
#include <functional>
#include "DynamicArray.hpp"

template<class T, class Compare = std::less<T>>
class BinaryHeap {
private:
    DynamicArray<T> items;
    Compare compare;

    void Swap(int left, int right);
    void SiftUp(int index);
    void SiftDown(int index);

public:
    BinaryHeap();
    explicit BinaryHeap(Compare customCompare);

    bool IsEmpty() const;
    int GetSize() const;
    const T& Peek() const;
    void Push(const T& item);
    T Pop();
};

#include "BinaryHeap.tpp"
