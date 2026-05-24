#pragma once
#include <functional>
#include "DynamicArray.hpp"

template<class T, class Compare = std::less<T>>
class BinaryHeap {
private:
    DynamicArray<T> items;
    Compare compare;

    void Swap(int left, int right) {
        T temp = items[left];
        items[left] = items[right];
        items[right] = temp;
    }

    void SiftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (!compare(items[index], items[parent])) {
                break;
            }
            Swap(index, parent);
            index = parent;
        }
    }

    void SiftDown(int index) {
        int size = items.GetSize();
        while (true) {
            int left = index * 2 + 1;
            int right = index * 2 + 2;
            int best = index;

            if (left < size && compare(items[left], items[best])) {
                best = left;
            }
            if (right < size && compare(items[right], items[best])) {
                best = right;
            }
            if (best == index) {
                break;
            }

            Swap(index, best);
            index = best;
        }
    }

public:
    BinaryHeap() : items(), compare(Compare()) {}

    explicit BinaryHeap(Compare customCompare) : items(), compare(customCompare) {}

    bool IsEmpty() const {
        return items.GetSize() == 0;
    }

    int GetSize() const {
        return items.GetSize();
    }

    const T& Peek() const {
        if (IsEmpty()) {
            throw EmptyCollection();
        }
        return items.Get(0);
    }

    void Push(const T& item) {
        items.Append(item);
        SiftUp(items.GetSize() - 1);
    }

    T Pop() {
        if (IsEmpty()) {
            throw EmptyCollection();
        }

        T top = items.Get(0);
        int lastIndex = items.GetSize() - 1;
        if (lastIndex == 0) {
            items.Resize(0);
            return top;
        }

        items[0] = items[lastIndex];
        items.Resize(lastIndex);
        SiftDown(0);
        return top;
    }
};
