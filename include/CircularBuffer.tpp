#pragma once

template<class T>
CircularBuffer<T>::CircularBuffer(size_t bufferCapacity)
    : data(nullptr), capacity(bufferCapacity), head(0), count(0) {
    if (bufferCapacity == 0) {
        throw InvalidArgument("circular buffer capacity must be positive");
    }
    data = new T[bufferCapacity];
}

template<class T>
CircularBuffer<T>::CircularBuffer(const CircularBuffer& other)
    : data(new T[other.capacity]),
      capacity(other.capacity),
      head(other.head),
      count(other.count) {
    for (size_t index = 0; index < capacity; index++) {
        data[index] = other.data[index];
    }
}

template<class T>
CircularBuffer<T>& CircularBuffer<T>::operator=(const CircularBuffer& other) {
    if (this != &other) {
        T* newData = new T[other.capacity];
        for (size_t index = 0; index < other.capacity; index++) {
            newData[index] = other.data[index];
        }
        delete[] data;
        data = newData;
        capacity = other.capacity;
        head = other.head;
        count = other.count;
    }
    return *this;
}

template<class T>
CircularBuffer<T>::CircularBuffer(CircularBuffer&& other) noexcept
    : data(other.data),
      capacity(other.capacity),
      head(other.head),
      count(other.count) {
    other.data = nullptr;
    other.capacity = 0;
    other.head = 0;
    other.count = 0;
}

template<class T>
CircularBuffer<T>& CircularBuffer<T>::operator=(CircularBuffer&& other) noexcept {
    if (this != &other) {
        delete[] data;
        data = other.data;
        capacity = other.capacity;
        head = other.head;
        count = other.count;
        other.data = nullptr;
        other.capacity = 0;
        other.head = 0;
        other.count = 0;
    }
    return *this;
}

template<class T>
CircularBuffer<T>::~CircularBuffer() {
    delete[] data;
}

template<class T>
Option<T> CircularBuffer<T>::AppendReturningEvicted(const T& value) {
    Option<T> evicted = Option<T>::None();
    if (count == capacity) {
        evicted = Option<T>::Some(data[head]);
    } else {
        count++;
    }

    data[head] = value;
    head = (head + 1) % capacity;
    return evicted;
}

template<class T>
T CircularBuffer<T>::Get(size_t index) const {
    if (index >= count) {
        throw IndexOutOfRange(static_cast<int>(index), static_cast<int>(count));
    }

    size_t start = count == capacity ? head : 0;
    size_t actualIndex = (start + index) % capacity;
    return data[actualIndex];
}

template<class T>
size_t CircularBuffer<T>::GetLength() const {
    return count;
}

template<class T>
size_t CircularBuffer<T>::GetCapacity() const {
    return capacity;
}

template<class T>
bool CircularBuffer<T>::IsEmpty() const {
    return count == 0;
}

template<class T>
void CircularBuffer<T>::Clear() {
    head = 0;
    count = 0;
}
