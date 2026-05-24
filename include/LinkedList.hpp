#pragma once
#include "Exceptions.hpp"
#include "IEnumerator.hpp"

template<class T>
class LinkedList {
private:
    struct Node {
        T value;
        Node* next;
        Node(const T& val) : value(val), next(nullptr) {}
    };

    Node* head;
    Node* tail;
    int length;

public:
    class Enumerator : public IEnumerator<T> {
    private:
        const LinkedList<T>* list;
        const Node* current;
        bool started;

    public:
        explicit Enumerator(const LinkedList<T>* source)
            : list(source), current(nullptr), started(false) {}

        bool MoveNext() override {
            if (!started) {
                current = list->head;
                started = true;
            } else if (current != nullptr) {
                current = current->next;
            }
            return current != nullptr;
        }

        const T& GetCurrent() const override {
            if (current == nullptr) {
                throw InvalidState("enumerator is not positioned on an element");
            }
            return current->value;
        }

        void Reset() override {
            current = nullptr;
            started = false;
        }
    };

private:
    void Clear() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        head = nullptr;
        tail = nullptr;
        length = 0;
    }

    Node* GetNode(int index) const {
        if (index < 0 || index >= length) {
            throw IndexOutOfRange(index, length);
        }
        Node* current = head;
        for (int i = 0; i < index; i++) {
            current = current->next;
        }
        return current;
    }

    void CopyFrom(const LinkedList<T>& other) {
        length = 0;
        head = nullptr;
        tail = nullptr;

        Node* current = other.head;
        while (current != nullptr) {
            Append(current->value);
            current = current->next;
        }
    }

public:
    LinkedList() : head(nullptr), tail(nullptr), length(0) {}

    LinkedList(const T* items, int count) : head(nullptr), tail(nullptr), length(0) {
        if (count < 0) {
            throw IndexOutOfRange(count, 0);
        }
        for (int i = 0; i < count; i++) {
            Append(items[i]);
        }
    }

    LinkedList(const LinkedList<T>& other) : head(nullptr), tail(nullptr), length(0) {
        CopyFrom(other);
    }

    LinkedList(LinkedList<T>&& other) noexcept
        : head(other.head), tail(other.tail), length(other.length) {
        other.head = nullptr;
        other.tail = nullptr;
        other.length = 0;
    }

    ~LinkedList() {
        Clear();
    }

    LinkedList<T>& operator=(const LinkedList<T>& other) {
        if (this != &other) {
            Clear();
            CopyFrom(other);
        }
        return *this;
    }

    LinkedList<T>& operator=(LinkedList<T>&& other) noexcept {
        if (this != &other) {
            Clear();
            head = other.head;
            tail = other.tail;
            length = other.length;
            other.head = nullptr;
            other.tail = nullptr;
            other.length = 0;
        }
        return *this;
    }

    const T& GetFirst() const {
        if (length == 0) {
            throw IndexOutOfRange(0, length);
        }
        return head->value;
    }

    const T& GetLast() const {
        if (length == 0) {
            throw IndexOutOfRange(0, length);
        }
        return tail->value;
    }

    const T& Get(int index) const {
        return GetNode(index)->value;
    }

    LinkedList<T>* GetSubList(int start, int end) const {
        if (start < 0 || end >= length || start > end) {
            throw IndexOutOfRange(start, length);
        }

        LinkedList<T>* result = new LinkedList<T>();
        Node* current = head;
        for (int i = 0; i < start; i++) {
            current = current->next;
        }
        for (int i = start; i <= end; i++) {
            result->Append(current->value);
            current = current->next;
        }
        return result;
    }

    int GetLength() const {
        return length;
    }

    void CopyToArray(T* destination) const {
        Node* current = head;
        int index = 0;
        while (current != nullptr) {
            destination[index++] = current->value;
            current = current->next;
        }
    }

    IEnumerator<T>* GetEnumerator() const {
        return new Enumerator(this);
    }

    void Append(const T& item) {
        Node* newNode = new Node(item);
        if (length == 0) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        length++;
    }

    void Prepend(const T& item) {
        Node* newNode = new Node(item);
        if (length == 0) {
            head = tail = newNode;
        } else {
            newNode->next = head;
            head = newNode;
        }
        length++;
    }

    void InsertAt(const T& item, int index) {
        if (index < 0 || index >= length) {
            throw IndexOutOfRange(index, length);
        }

        if (index == 0) {
            Prepend(item);
            return;
        }

        Node* prev = GetNode(index - 1);
        Node* newNode = new Node(item);
        newNode->next = prev->next;
        prev->next = newNode;
        length++;
    }

    LinkedList<T>* Concat(const LinkedList<T>* list) const {
        LinkedList<T>* result = new LinkedList<T>(*this);
        Node* current = list->head;
        while (current != nullptr) {
            result->Append(current->value);
            current = current->next;
        }
        return result;
    }

    T& operator[](int index) {
        return GetNode(index)->value;
    }

    const T& operator[](int index) const {
        return GetNode(index)->value;
    }
};
