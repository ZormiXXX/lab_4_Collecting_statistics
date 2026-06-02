#pragma once

template<class T, class Compare>
void BinaryHeap<T, Compare>::Swap(int left, int right) {
    T temp = items[left];
    items[left] = items[right];
    items[right] = temp;
}

template<class T, class Compare>
void BinaryHeap<T, Compare>::SiftUp(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (!compare(items[index], items[parent])) {
            break;
        }
        Swap(index, parent);
        index = parent;
    }
}

template<class T, class Compare>
void BinaryHeap<T, Compare>::SiftDown(int index) {
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

template<class T, class Compare>
BinaryHeap<T, Compare>::BinaryHeap() : items(), compare(Compare()) {}

template<class T, class Compare>
BinaryHeap<T, Compare>::BinaryHeap(Compare customCompare) : items(), compare(customCompare) {}

template<class T, class Compare>
bool BinaryHeap<T, Compare>::IsEmpty() const {
    return items.GetSize() == 0;
}

template<class T, class Compare>
int BinaryHeap<T, Compare>::GetSize() const {
    return items.GetSize();
}

template<class T, class Compare>
const T& BinaryHeap<T, Compare>::Peek() const {
    if (IsEmpty()) {
        throw EmptyCollection();
    }
    return items.Get(0);
}

template<class T, class Compare>
void BinaryHeap<T, Compare>::Push(const T& item) {
    items.Append(item);
    SiftUp(items.GetSize() - 1);
}

template<class T, class Compare>
T BinaryHeap<T, Compare>::Pop() {
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
