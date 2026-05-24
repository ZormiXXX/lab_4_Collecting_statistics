#pragma once
#include "Exceptions.hpp"
#include "IEnumerator.hpp"

template<class T>
class DynamicArray {
private:
    T* data;
    int size;
    int capacity;

    void EnsureCapacity(int minCapacity) {
        if (capacity >= minCapacity) return;

        int newCapacity = capacity == 0 ? 4 : capacity * 2;
        while (newCapacity < minCapacity) newCapacity *= 2;

        T* newData = new T[newCapacity];
        for (int i = 0; i < size; i++) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }

public:
    class Enumerator : public IEnumerator<T> {
    private:
        const DynamicArray<T>* array;
        int index;

    public:
        explicit Enumerator(const DynamicArray<T>* source) : array(source), index(-1) {}

        bool MoveNext() override {
            if (index + 1 >= array->size) {
                return false;
            }
            index++;
            return true;
        }

        const T& GetCurrent() const override {
            if (index < 0 || index >= array->size) {
                throw IndexOutOfRange(index, array->size);
            }
            return array->data[index];
        }

        void Reset() override {
            index = -1;
        }
    };

    DynamicArray() : data(nullptr), size(0), capacity(0) {}

    DynamicArray(const T* items, int count) : size(count), capacity(count) {
        if (count < 0) {
            throw IndexOutOfRange(count, 0);
        }
        data = count > 0 ? new T[count] : nullptr;
        for (int i = 0; i < count; i++) {
            data[i] = items[i];
        }
    }

    DynamicArray(int size) : size(size), capacity(size) {
        if (size < 0) {
            throw IndexOutOfRange(size, 0);
        }
        data = size > 0 ? new T[size] : nullptr;
        for (int i = 0; i < size; i++) {
            data[i] = T();
        }
    }

    DynamicArray(const DynamicArray<T>& other) : size(other.size), capacity(other.capacity) {
        data = capacity > 0 ? new T[capacity] : nullptr;
        for (int i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }

    DynamicArray(const DynamicArray<T>& other, int minCapacity) : size(other.size), capacity(other.capacity) {
        if (minCapacity < 0) {
            throw IndexOutOfRange(minCapacity, 0);
        }
        if (capacity < minCapacity) {
            capacity = minCapacity;
        }
        if (capacity < size) {
            capacity = size;
        }
        data = capacity > 0 ? new T[capacity] : nullptr;
        for (int i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }

    DynamicArray(DynamicArray<T>&& other) noexcept
        : data(other.data), size(other.size), capacity(other.capacity) {
        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
    }

    ~DynamicArray() {
        delete[] data;
    }

    DynamicArray<T>& operator=(const DynamicArray<T>& other) {
        if (this != &other) {
            T* newData = other.capacity > 0 ? new T[other.capacity] : nullptr;
            for (int i = 0; i < other.size; i++) {
                newData[i] = other.data[i];
            }
            delete[] data;
            data = newData;
            size = other.size;
            capacity = other.capacity;
        }
        return *this;
    }

    DynamicArray<T>& operator=(DynamicArray<T>&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            size = other.size;
            capacity = other.capacity;
            other.data = nullptr;
            other.size = 0;
            other.capacity = 0;
        }
        return *this;
    }

    const T& Get(int index) const {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        return data[index];
    }

    void Set(int index, const T& value) {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        data[index] = value;
    }

    void Reserve(int minCapacity) {
        if (minCapacity < 0) {
            throw IndexOutOfRange(minCapacity, 0);
        }
        EnsureCapacity(minCapacity);
    }

    void Append(const T& value) {
        EnsureCapacity(size + 1);
        data[size] = value;
        size++;
    }

    void InsertAt(int index, const T& value) {
        if (index < 0 || index > size) {
            throw IndexOutOfRange(index, size);
        }

        EnsureCapacity(size + 1);
        for (int i = size; i > index; i--) {
            data[i] = data[i - 1];
        }
        data[index] = value;
        size++;
    }

    int GetSize() const {
        return size;
    }

    int GetCapacity() const {
        return capacity;
    }

    T* RawData() {
        return data;
    }

    const T* RawData() const {
        return data;
    }

    IEnumerator<T>* GetEnumerator() const {
        return new Enumerator(this);
    }

    void Resize(int newSize) {
        if (newSize < 0) throw IndexOutOfRange(newSize, 0);

        EnsureCapacity(newSize);

        if (newSize > size) {
            for (int i = size; i < newSize; i++) {
                data[i] = T();
            }
        }
        size = newSize;
    }

    T& operator[](int index) {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        return data[index];
    }

    const T& operator[](int index) const {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        return data[index];
    }
};
