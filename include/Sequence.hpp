#pragma once
#include "Exceptions.hpp"
#include "IEnumerable.hpp"
#include "Option.hpp"

template<class T>
class ArraySequence;

template<class T>
class Sequence;

template<class T, class Derived>
class SequenceCRTP;

template<class First, class Second>
struct Tuple {
    First first;
    Second second;

    Tuple() : first(), second() {}

    Tuple(const First& firstValue, const Second& secondValue)
        : first(firstValue), second(secondValue) {}
};

template<class TupleType>
struct TupleTraits;

template<class First, class Second>
struct TupleTraits<Tuple<First, Second>> {
    using FirstType = First;
    using SecondType = Second;
};

template<class First, class Second>
struct SequencePair {
    Sequence<First>* first;
    Sequence<Second>* second;
};

template<class T>
class SequenceEnumerator : public IEnumerator<T> {
private:
    const Sequence<T>* sequence;
    int index;

public:
    explicit SequenceEnumerator(const Sequence<T>* seq) : sequence(seq), index(-1) {}

    bool MoveNext() override;
    const T& GetCurrent() const override;
    void Reset() override;
};

template<class T>
class Sequence : public IEnumerable<T> {
protected:
    virtual Sequence<T>* CreateAccumulator() const {
        return CreateEmpty();
    }

    virtual Sequence<T>* CreateAccumulator(int expectedLength) const {
        (void)expectedLength;
        return CreateAccumulator();
    }

    virtual Sequence<T>* FinalizeAccumulator(Sequence<T>* accumulator) const {
        return accumulator;
    }

public:
    virtual ~Sequence() = default;

    virtual const T& Get(int index) const = 0;
    virtual int GetLength() const = 0;
    virtual const T& GetFirst() const = 0;
    virtual const T& GetLast() const = 0;

    virtual Sequence<T>* Append(const T& item) = 0;
    virtual Sequence<T>* Prepend(const T& item) = 0;
    virtual Sequence<T>* InsertAt(const T& item, int index) = 0;
    virtual Sequence<T>* Concat(const Sequence<T>* other) = 0;
    virtual Sequence<T>* GetSubsequence(int start, int end) const = 0;
    virtual Sequence<T>* CreateEmpty() const = 0;
    virtual Sequence<T>* CreateFromArray(const T* items, int count) const = 0;

    IEnumerator<T>* GetEnumerator() const override;

    template<typename Func>
    Sequence<T>* Map(Func f) const {
        Sequence<T>* result = CreateAccumulator(GetLength());
        IEnumerator<T>* enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            Sequence<T>* updated = result->Append(f(enumerator->GetCurrent()));
            if (updated != result) {
                delete result;
            }
            result = updated;
        }
        delete enumerator;
        return FinalizeAccumulator(result);
    }

    template<typename Func>
    Sequence<T>* FlatMap(Func f) const {
        Sequence<T>* result = CreateAccumulator(GetLength());
        IEnumerator<T>* enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            Sequence<T>* expanded = f(enumerator->GetCurrent());
            Sequence<T>* updated = result->Concat(expanded);
            if (updated != result) {
                delete result;
            }
            result = updated;
            delete expanded;
        }
        delete enumerator;
        return FinalizeAccumulator(result);
    }

    template<typename Func, typename U>
    U Reduce(Func f, U initial) const {
        U result = initial;
        IEnumerator<T>* enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            result = f(enumerator->GetCurrent(), result);
        }
        delete enumerator;
        return result;
    }

    template<typename Predicate>
    Sequence<T>* Where(Predicate pred) const {
        Sequence<T>* result = CreateAccumulator(GetLength());
        IEnumerator<T>* enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& item = enumerator->GetCurrent();
            if (pred(item)) {
                Sequence<T>* updated = result->Append(item);
                if (updated != result) {
                    delete result;
                }
                result = updated;
            }
        }
        delete enumerator;
        return FinalizeAccumulator(result);
    }

    template<typename Predicate>
    T Find(Predicate pred) const {
        IEnumerator<T>* enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& item = enumerator->GetCurrent();
            if (pred(item)) {
                delete enumerator;
                return item;
            }
        }
        delete enumerator;
        throw ElementNotFound();
    }

    template<typename U>
    Sequence<Tuple<T, U>>* Zip(const Sequence<U>* other) const {
        if (other == nullptr) {
            throw InvalidArgument("other sequence must not be null");
        }

        int leftLength = GetLength();
        int rightLength = other->GetLength();
        int resultLength = leftLength < rightLength ? leftLength : rightLength;
        auto* result = new ArraySequence<Tuple<T, U>>(resultLength);
        IEnumerator<T>* left = GetEnumerator();
        IEnumerator<U>* right = other->GetEnumerator();

        while (left->MoveNext() && right->MoveNext()) {
            result->Append(Tuple<T, U>(left->GetCurrent(), right->GetCurrent()));
        }

        delete left;
        delete right;
        return result;
    }

    template<
        typename TupleType = T,
        typename First = typename TupleTraits<TupleType>::FirstType,
        typename Second = typename TupleTraits<TupleType>::SecondType
    >
    SequencePair<First, Second> Unzip() const {
        int length = GetLength();
        auto* first = new ArraySequence<First>(length);
        auto* second = new ArraySequence<Second>(length);

        IEnumerator<T>* enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const TupleType& item = enumerator->GetCurrent();
            first->Append(item.first);
            second->Append(item.second);
        }
        delete enumerator;

        return {first, second};
    }

    template<typename Predicate>
    Sequence<Sequence<T>*>* Split(Predicate pred) const {
        int length = GetLength();
        auto* result = new ArraySequence<Sequence<T>*>(length + 1);
        Sequence<T>* current = CreateAccumulator(length);
        IEnumerator<T>* enumerator = GetEnumerator();

        while (enumerator->MoveNext()) {
            const T& item = enumerator->GetCurrent();
            if (pred(item)) {
                if (current->GetLength() > 0) {
                    result->Append(FinalizeAccumulator(current));
                    current = CreateAccumulator(length);
                }
            } else {
                Sequence<T>* updated = current->Append(item);
                if (updated != current) {
                    delete current;
                }
                current = updated;
            }
        }
        delete enumerator;

        if (current->GetLength() > 0) {
            result->Append(FinalizeAccumulator(current));
        } else {
            delete current;
        }

        return result;
    }

    Sequence<T>* Slice(int index, int count, const Sequence<T>* replacement = nullptr) const {
        if (count < 0) {
            throw InvalidArgument("count must be non-negative");
        }

        int length = GetLength();
        int actualIndex = index;
        if (actualIndex < 0) {
            actualIndex = length + actualIndex;
        }

        if (actualIndex < 0 || actualIndex > length) {
            throw IndexOutOfRange(index, length);
        }

        int skipUntil = actualIndex + count;
        if (skipUntil < actualIndex) {
            skipUntil = length;
        }
        if (skipUntil > length) {
            skipUntil = length;
        }

        int replacementLength = replacement == nullptr ? 0 : replacement->GetLength();
        int removedLength = skipUntil - actualIndex;
        Sequence<T>* result = CreateAccumulator(length - removedLength + replacementLength);
        IEnumerator<T>* source = GetEnumerator();
        int currentIndex = 0;
        bool replacementInserted = false;

        while (source->MoveNext()) {
            if (!replacementInserted && currentIndex == actualIndex && replacement != nullptr) {
                IEnumerator<T>* replacementEnumerator = replacement->GetEnumerator();
                while (replacementEnumerator->MoveNext()) {
                    Sequence<T>* updated = result->Append(replacementEnumerator->GetCurrent());
                    if (updated != result) {
                        delete result;
                    }
                    result = updated;
                }
                delete replacementEnumerator;
                replacementInserted = true;
            }

            const T& item = source->GetCurrent();
            if (currentIndex < actualIndex || currentIndex >= skipUntil) {
                Sequence<T>* updated = result->Append(item);
                if (updated != result) {
                    delete result;
                }
                result = updated;
            }
            currentIndex++;
        }
        delete source;

        if (!replacementInserted && replacement != nullptr) {
            IEnumerator<T>* replacementEnumerator = replacement->GetEnumerator();
            while (replacementEnumerator->MoveNext()) {
                Sequence<T>* updated = result->Append(replacementEnumerator->GetCurrent());
                if (updated != result) {
                    delete result;
                }
                result = updated;
            }
            delete replacementEnumerator;
        }

        return FinalizeAccumulator(result);
    }

    Option<T> TryGet(int index) const {
        if (index < 0 || index >= GetLength()) {
            return Option<T>::None();
        }
        return Option<T>::Some(Get(index));
    }

    template<typename Predicate>
    Option<T> TryFind(Predicate pred) const {
        IEnumerator<T>* enumerator = GetEnumerator();
        while (enumerator->MoveNext()) {
            const T& item = enumerator->GetCurrent();
            if (pred(item)) {
                delete enumerator;
                return Option<T>::Some(item);
            }
        }
        delete enumerator;
        return Option<T>::None();
    }

    Option<T> TryFirst() const {
        if (GetLength() == 0) {
            return Option<T>::None();
        }
        return Option<T>::Some(GetFirst());
    }

    Option<T> TryLast() const {
        if (GetLength() == 0) {
            return Option<T>::None();
        }
        return Option<T>::Some(GetLast());
    }

    const T& operator[](int index) const {
        return Get(index);
    }

    static Sequence<T>* From(const T* arr, int count) {
        ArraySequence<T> factory;
        return factory.CreateFromArray(arr, count);
    }
};

template<class T, class Derived>
class SequenceCRTP : public Sequence<T> {
private:
    Derived* Self() {
        return static_cast<Derived*>(this);
    }

public:
    Sequence<T>* Append(const T& item) override {
        Derived* result = Self()->Instance(this->GetLength() + 1);
        result->AppendInternal(item);
        return result;
    }

    Sequence<T>* Prepend(const T& item) override {
        Derived* result = Self()->Instance(this->GetLength() + 1);
        result->PrependInternal(item);
        return result;
    }

    Sequence<T>* InsertAt(const T& item, int index) override {
        Derived* result = Self()->Instance(this->GetLength() + 1);
        result->InsertAtInternal(item, index);
        return result;
    }

    Sequence<T>* Concat(const Sequence<T>* other) override {
        if (other == nullptr) {
            throw InvalidArgument("other sequence must not be null");
        }
        Derived* result = Self()->Instance(this->GetLength() + other->GetLength());
        result->ConcatInternal(other);
        return result;
    }
};

template<class T>
bool SequenceEnumerator<T>::MoveNext() {
    if (index + 1 >= sequence->GetLength()) {
        return false;
    }
    index++;
    return true;
}

template<class T>
const T& SequenceEnumerator<T>::GetCurrent() const {
    if (index < 0 || index >= sequence->GetLength()) {
        throw IndexOutOfRange(index, sequence->GetLength());
    }
    return sequence->Get(index);
}

template<class T>
void SequenceEnumerator<T>::Reset() {
    index = -1;
}

template<class T>
IEnumerator<T>* Sequence<T>::GetEnumerator() const {
    return new SequenceEnumerator<T>(this);
}
