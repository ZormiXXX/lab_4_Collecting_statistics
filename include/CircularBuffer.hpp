#pragma once
#include "Option.hpp"
#include "Exceptions.hpp"

template<class T>
class CircularBuffer {
private:
    T* data;
    size_t capacity;
    size_t head;
    size_t count;

public:
    explicit CircularBuffer(size_t bufferCapacity);
    CircularBuffer(const CircularBuffer& other);
    CircularBuffer& operator=(const CircularBuffer& other);
    CircularBuffer(CircularBuffer&& other) noexcept;
    CircularBuffer& operator=(CircularBuffer&& other) noexcept;
    ~CircularBuffer();

    Option<T> AppendReturningEvicted(const T& value);
    T Get(size_t index) const;
    size_t GetLength() const;
    size_t GetCapacity() const;
    bool IsEmpty() const;
    void Clear();
};

#include "CircularBuffer.tpp"
