#pragma once

#include "../SegmentedDeque.hpp"

template<class T>
class ImmutableSegmentedDeque : public SegmentedDeque<T> {
protected:
    ImmutableSegmentedDeque<T>* Instance() override {
        return new ImmutableSegmentedDeque<T>(*this);
    }

    ImmutableSegmentedDeque<T>* CreateEmptySameKind() const override {
        return new ImmutableSegmentedDeque<T>(this->blockCapacity, this->storageKind);
    }

public:
    explicit ImmutableSegmentedDeque(
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDeque<T>(blockCapacityValue, kind) {}

    ImmutableSegmentedDeque(
        const T* items,
        int count,
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDeque<T>(items, count, blockCapacityValue, kind) {}

    ImmutableSegmentedDeque(const ImmutableSegmentedDeque<T>& other)
        : SegmentedDeque<T>(other) {}
};
