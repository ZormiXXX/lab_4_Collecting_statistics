#pragma once

#include "MapIndexed.hpp"
#include "Mutable/MutableListSequence.hpp"
#include "Mutable/MutableSegmentedDeque.hpp"
#include "SegmentedDeque.hpp"
#include <functional>
#include <random>

template<class T, class Compare = std::less<T>>
long long CountInversionsMapReduce(const SegmentedDeque<T>& deque, Compare compare = Compare{}) {
    Sequence<T>* flat = deque.ToSequence();
    Sequence<long long>* localCounts = MapIndexed<T, long long>(
        *flat,
        [&](const T& value, int index) {
            long long local = 0;
            for (int j = index + 1; j < flat->GetLength(); j++) {
                if (compare(flat->Get(j), value)) {
                    local++;
                }
            }
            return local;
        }
    );

    long long total = localCounts->Reduce(
        [](long long value, long long acc) { return value + acc; },
        0LL
    );

    delete localCounts;
    delete flat;
    return total;
}

template<class T, class Compare = std::less<T>>
long long CountInversionsMultiPass(const SegmentedDeque<T>& deque, Compare compare = Compare{}) {
    long long total = 0;
    for (int i = 0; i < deque.GetLength(); i++) {
        for (int j = i + 1; j < deque.GetLength(); j++) {
            if (compare(deque.Get(j), deque.Get(i))) {
                total++;
            }
        }
    }
    return total;
}

template<class T, class Compare = std::less<T>>
long long CountInversionsOnePass(const SegmentedDeque<T>& deque, Compare compare = Compare{}) {
    MutableListSequence<T> sortedSeen;
    long long total = 0;

    for (int i = 0; i < deque.GetLength(); i++) {
        T value = deque.Get(i);
        int insertPosition = 0;

        while (insertPosition < sortedSeen.GetLength() &&
               !compare(value, sortedSeen.Get(insertPosition))) {
            insertPosition++;
        }

        total += sortedSeen.GetLength() - insertPosition;

        if (sortedSeen.GetLength() == 0) {
            sortedSeen.Append(value);
        } else if (insertPosition == 0) {
            sortedSeen.Prepend(value);
        } else if (insertPosition == sortedSeen.GetLength()) {
            sortedSeen.Append(value);
        } else {
            sortedSeen.InsertAt(value, insertPosition);
        }
    }

    return total;
}

inline MutableSegmentedDeque<int>* GenerateRandomIntDeque(
    int count,
    int blockCapacity,
    int minValue,
    int maxValue,
    unsigned int seed,
    DequeStorageKind storageKind = DequeStorageKind::ArraySequence
) {
    auto* result = new MutableSegmentedDeque<int>(blockCapacity, storageKind);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(minValue, maxValue);

    for (int i = 0; i < count; i++) {
        result->PushBack(distribution(generator));
    }

    return result;
}
