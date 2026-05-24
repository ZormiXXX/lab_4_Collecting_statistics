#pragma once

#include "../SegmentedDeque.hpp"

template<class T>
class MutableSegmentedDeque : public SegmentedDeque<T> {
protected:
    MutableSegmentedDeque<T>* Instance() override {
        return this;
    }

    MutableSegmentedDeque<T>* CreateEmptySameKind() const override {
        return new MutableSegmentedDeque<T>(this->blockCapacity, this->storageKind);
    }

public:
    explicit MutableSegmentedDeque(
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDeque<T>(blockCapacityValue, kind) {}

    MutableSegmentedDeque(
        const T* items,
        int count,
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDeque<T>(items, count, blockCapacityValue, kind) {}

    MutableSegmentedDeque(const MutableSegmentedDeque<T>& other)
        : SegmentedDeque<T>(other) {}
};
