#pragma once
#include <functional>
#include <memory>
#include "ArraySequence.hpp"
#include "Cardinal.hpp"

template<class T>
class LazySequence {
    template<class>
    friend class LazySequence;

public:
    using NextFunction = std::function<T(size_t, const Sequence<T>*)>;
    using LengthResolver = std::function<Cardinal()>;

    class Generator {
    private:
        const LazySequence<T>* owner;
        size_t position;

    public:
        explicit Generator(const LazySequence<T>& sequence, size_t startPosition = 0)
            : owner(&sequence), position(startPosition) {}

        bool HasNext() const {
            Cardinal length = owner->GetLength();
            return length.IsInfinite() || position < length.AsFinite();
        }

        T GetNext() {
            T value = owner->Get(static_cast<int>(position));
            position++;
            return value;
        }

        Option<T> TryGetNext() {
            try {
                return Option<T>::Some(GetNext());
            } catch (const Exception&) {
                return Option<T>::None();
            }
        }
    };

private:
    struct State {
        Cardinal lengthHint;
        Cardinal exactLength;
        bool exactLengthKnown;
        bool finished;
        LengthResolver lengthResolver;
        NextFunction nextFunction;
        mutable ArraySequence<T> materialized;

        State()
            : lengthHint(Cardinal::Finite(0)),
              exactLength(Cardinal::Finite(0)),
              exactLengthKnown(true),
              finished(true),
              lengthResolver(),
              nextFunction(),
              materialized() {}
    };

    std::shared_ptr<State> state;

    explicit LazySequence(std::shared_ptr<State> sharedState) : state(std::move(sharedState)) {}

    static LazySequence<T> Create(
        const Cardinal& lengthHint,
        const LengthResolver& lengthResolver,
        const NextFunction& nextFunction,
        bool finished = false,
        bool exactLengthKnown = true
    ) {
        auto sharedState = std::make_shared<State>();
        sharedState->lengthHint = lengthHint;
        sharedState->exactLength = lengthHint.IsFinite() ? lengthHint : Cardinal::Finite(0);
        sharedState->exactLengthKnown = exactLengthKnown;
        sharedState->finished = finished;
        sharedState->lengthResolver = lengthResolver;
        sharedState->nextFunction = nextFunction;
        return LazySequence<T>(sharedState);
    }

    void EnsureCanIndex(int index) const {
        if (index < 0) {
            throw IndexOutOfRange(index, state->materialized.GetLength());
        }
    }

    void EnsureMaterialized(size_t index) const {
        if (state->finished && index >= static_cast<size_t>(state->materialized.GetLength())) {
            throw IndexOutOfRange(static_cast<int>(index), state->materialized.GetLength());
        }

        while (static_cast<size_t>(state->materialized.GetLength()) <= index) {
            if (state->exactLengthKnown &&
                state->exactLength.IsFinite() &&
                static_cast<size_t>(state->materialized.GetLength()) >= state->exactLength.AsFinite()) {
                state->finished = true;
                throw IndexOutOfRange(static_cast<int>(index), state->materialized.GetLength());
            }

            try {
                T value = state->nextFunction(
                    static_cast<size_t>(state->materialized.GetLength()),
                    &state->materialized
                );
                state->materialized.Append(value);
            } catch (const IndexOutOfRange&) {
                state->finished = true;
                state->exactLengthKnown = true;
                state->exactLength = Cardinal::Finite(state->materialized.GetLength());
                throw IndexOutOfRange(static_cast<int>(index), state->materialized.GetLength());
            }
        }
    }

    void MaterializeAllFinite() const {
        Cardinal length = GetLength();
        if (length.IsInfinite()) {
            throw InfinityError("cannot fully materialize an infinite lazy sequence");
        }

        size_t target = length.AsFinite();
        if (target == 0) {
            state->finished = true;
            return;
        }

        EnsureMaterialized(target - 1);
        state->finished = true;
        state->exactLengthKnown = true;
        state->exactLength = Cardinal::Finite(target);
    }

public:
    LazySequence() : state(std::make_shared<State>()) {}

    LazySequence(const T* items, int count) : state(std::make_shared<State>()) {
        if (count < 0) {
            throw InvalidArgument("count must be non-negative");
        }

        for (int index = 0; index < count; index++) {
            state->materialized.Append(items[index]);
        }

        state->lengthHint = Cardinal::Finite(count);
        state->exactLength = Cardinal::Finite(count);
        state->exactLengthKnown = true;
        state->finished = true;
        state->nextFunction = [](size_t, const Sequence<T>*) -> T {
            throw IndexOutOfRange(0, 0);
        };
    }

    explicit LazySequence(const Sequence<T>* sequence) : state(std::make_shared<State>()) {
        if (sequence == nullptr) {
            throw InvalidArgument("sequence must not be null");
        }

        int length = sequence->GetLength();
        state->lengthHint = Cardinal::Finite(length);
        state->exactLength = Cardinal::Finite(length);
        state->exactLengthKnown = true;
        state->finished = false;

        auto source = std::make_shared<ArraySequence<T>>();
        for (int index = 0; index < length; index++) {
            source->Append(sequence->Get(index));
        }

        state->nextFunction = [source](size_t index, const Sequence<T>*) -> T {
            return source->Get(static_cast<int>(index));
        };
    }

    LazySequence(const NextFunction& recurrenceRule, const Sequence<T>* seed)
        : state(std::make_shared<State>()) {
        if (seed == nullptr) {
            throw InvalidArgument("seed must not be null");
        }

        int seedLength = seed->GetLength();
        if (seedLength < 0) {
            throw InvalidArgument("seed length must be non-negative");
        }

        auto seedValues = std::make_shared<ArraySequence<T>>();
        for (int index = 0; index < seedLength; index++) {
            seedValues->Append(seed->Get(index));
        }

        state->lengthHint = Cardinal::Infinite();
        state->exactLengthKnown = false;
        state->finished = false;
        state->nextFunction = [seedValues, recurrenceRule](size_t index, const Sequence<T>* materialized) -> T {
            if (index < static_cast<size_t>(seedValues->GetLength())) {
                return seedValues->Get(static_cast<int>(index));
            }
            return recurrenceRule(index, materialized);
        };
    }

    static LazySequence<T> GenerateIndexed(
        const std::function<T(size_t)>& generator,
        const Cardinal& lengthHint = Cardinal::Infinite()
    ) {
        return Create(
            lengthHint,
            LengthResolver(),
            [generator](size_t index, const Sequence<T>*) -> T {
                return generator(index);
            }
        );
    }

    T Get(int index) const {
        EnsureCanIndex(index);
        EnsureMaterialized(static_cast<size_t>(index));
        return state->materialized.Get(index);
    }

    T GetFirst() const {
        return Get(0);
    }

    T GetLast() const {
        Cardinal length = GetLength();
        if (length.IsInfinite()) {
            throw InfinityError("cannot get the last element of an infinite lazy sequence");
        }
        if (length.AsFinite() == 0) {
            throw IndexOutOfRange(0, 0);
        }
        return Get(static_cast<int>(length.AsFinite() - 1));
    }

    Cardinal GetLength() const {
        if (state->exactLengthKnown) {
            return state->exactLength;
        }
        if (!state->lengthResolver) {
            return state->lengthHint;
        }

        Cardinal resolved = state->lengthResolver();
        if (resolved.IsFinite()) {
            state->exactLengthKnown = true;
            state->exactLength = resolved;
        }
        return resolved;
    }

    size_t GetMaterializedCount() const {
        return static_cast<size_t>(state->materialized.GetLength());
    }

    bool IsInfinite() const {
        return GetLength().IsInfinite();
    }

    ArraySequence<T>* ToSequence(size_t limit = 0, bool useLimit = false) const {
        auto* result = new ArraySequence<T>();

        if (useLimit) {
            for (size_t index = 0; index < limit; index++) {
                result->Append(Get(static_cast<int>(index)));
            }
            return result;
        }

        MaterializeAllFinite();
        for (int index = 0; index < state->materialized.GetLength(); index++) {
            result->Append(state->materialized.Get(index));
        }
        return result;
    }

    LazySequence<T> GetSubsequence(int startIndex, int endIndex) const {
        if (startIndex < 0 || endIndex < startIndex) {
            throw IndexOutOfRange(startIndex, state->materialized.GetLength());
        }

        Cardinal length = GetLength();
        if (length.IsFinite() && static_cast<size_t>(endIndex) >= length.AsFinite()) {
            throw IndexOutOfRange(endIndex, static_cast<int>(length.AsFinite()));
        }

        LazySequence<T> source = *this;
        size_t start = static_cast<size_t>(startIndex);
        size_t count = static_cast<size_t>(endIndex - startIndex + 1);

        return Create(
            Cardinal::Finite(count),
            [count]() {
                return Cardinal::Finite(count);
            },
            [source, start](size_t index, const Sequence<T>*) mutable -> T {
                return source.Get(static_cast<int>(start + index));
            }
        );
    }

    LazySequence<T> Take(size_t count) const {
        if (count == 0) {
            return LazySequence<T>();
        }
        return GetSubsequence(0, static_cast<int>(count - 1));
    }

    LazySequence<T> Prepend(const T& item) const {
        LazySequence<T> source = *this;
        Cardinal sourceLength = source.GetLength();
        Cardinal newLength = sourceLength.IsInfinite() ? Cardinal::Infinite() : sourceLength.Add(1);

        return Create(
            newLength,
            [newLength]() { return newLength; },
            [source, item](size_t index, const Sequence<T>*) mutable -> T {
                return index == 0 ? item : source.Get(static_cast<int>(index - 1));
            }
        );
    }

    LazySequence<T> Concat(const LazySequence<T>& other) const {
        LazySequence<T> left = *this;
        LazySequence<T> right = other;
        Cardinal leftLength = left.GetLength();

        if (leftLength.IsInfinite()) {
            return Create(
                Cardinal::Infinite(),
                []() { return Cardinal::Infinite(); },
                [left](size_t index, const Sequence<T>*) mutable -> T {
                    return left.Get(static_cast<int>(index));
                }
            );
        }

        Cardinal rightLength = right.GetLength();
        Cardinal totalLength = Cardinal::Add(leftLength, rightLength);
        size_t border = leftLength.AsFinite();

        return Create(
            totalLength,
            [totalLength]() { return totalLength; },
            [left, right, border](size_t index, const Sequence<T>*) mutable -> T {
                if (index < border) {
                    return left.Get(static_cast<int>(index));
                }
                return right.Get(static_cast<int>(index - border));
            }
        );
    }

    LazySequence<T> Append(const T& item) const {
        T singleItem[1] = {item};
        LazySequence<T> tail(singleItem, 1);
        return Concat(tail);
    }

    LazySequence<T> InsertAt(const T& item, int index) const {
        if (index < 0) {
            throw IndexOutOfRange(index, static_cast<int>(GetMaterializedCount()));
        }

        LazySequence<T> source = *this;
        Cardinal sourceLength = source.GetLength();
        if (sourceLength.IsFinite() && static_cast<size_t>(index) > sourceLength.AsFinite()) {
            throw IndexOutOfRange(index, static_cast<int>(sourceLength.AsFinite()));
        }

        Cardinal newLength = sourceLength.IsInfinite() ? Cardinal::Infinite() : sourceLength.Add(1);
        size_t insertIndex = static_cast<size_t>(index);

        return Create(
            newLength,
            [newLength]() { return newLength; },
            [source, item, insertIndex](size_t current, const Sequence<T>*) mutable -> T {
                if (current == insertIndex) {
                    return item;
                }
                if (current < insertIndex) {
                    return source.Get(static_cast<int>(current));
                }
                return source.Get(static_cast<int>(current - 1));
            }
        );
    }

    template<class U, class Mapper>
    LazySequence<U> Map(Mapper mapper) const {
        LazySequence<T> source = *this;
        Cardinal length = source.GetLength();

        return LazySequence<U>::Create(
            length,
            [length]() { return length; },
            [source, mapper](size_t index, const Sequence<U>*) mutable -> U {
                return mapper(source.Get(static_cast<int>(index)));
            }
        );
    }

    template<class Reducer, class U>
    U Reduce(Reducer reducer, U initial) const {
        Cardinal length = GetLength();
        if (length.IsInfinite()) {
            throw InfinityError("Reduce requires a finite lazy sequence");
        }

        U result = initial;
        size_t total = length.AsFinite();
        for (size_t index = 0; index < total; index++) {
            result = reducer(result, Get(static_cast<int>(index)));
        }
        return result;
    }

    template<class Reducer, class U>
    U ReduceFirst(Reducer reducer, U initial, size_t count) const {
        U result = initial;
        for (size_t index = 0; index < count; index++) {
            result = reducer(result, Get(static_cast<int>(index)));
        }
        return result;
    }

    template<class Predicate>
    LazySequence<T> Where(Predicate predicate) const {
        LazySequence<T> source = *this;
        Cardinal sourceLength = source.GetLength();
        auto nextSourceIndex = std::make_shared<size_t>(0);

        return Create(
            sourceLength.IsInfinite() ? Cardinal::Infinite() : Cardinal::Finite(0),
            [source, predicate, sourceLength]() mutable -> Cardinal {
                if (sourceLength.IsInfinite()) {
                    return Cardinal::Infinite();
                }

                size_t matches = 0;
                size_t total = sourceLength.AsFinite();
                for (size_t index = 0; index < total; index++) {
                    if (predicate(source.Get(static_cast<int>(index)))) {
                        matches++;
                    }
                }
                return Cardinal::Finite(matches);
            },
            [source, predicate, nextSourceIndex](size_t, const Sequence<T>*) mutable -> T {
                while (true) {
                    T candidate = source.Get(static_cast<int>(*nextSourceIndex));
                    (*nextSourceIndex)++;
                    if (predicate(candidate)) {
                        return candidate;
                    }
                }
            },
            false,
            sourceLength.IsInfinite()
        );
    }

    template<class U>
    LazySequence<Tuple<T, U>> Zip(const Sequence<U>& other) const {
        LazySequence<T> source = *this;
        Cardinal length = Cardinal::Min(source.GetLength(), Cardinal::Finite(other.GetLength()));
        auto copied = std::make_shared<ArraySequence<U>>();
        for (int index = 0; index < other.GetLength(); index++) {
            copied->Append(other.Get(index));
        }

        return LazySequence<Tuple<T, U>>::Create(
            length,
            [length]() { return length; },
            [source, copied](size_t index, const Sequence<Tuple<T, U>>*) mutable -> Tuple<T, U> {
                return Tuple<T, U>(
                    source.Get(static_cast<int>(index)),
                    copied->Get(static_cast<int>(index))
                );
            }
        );
    }

    template<class U>
    LazySequence<Tuple<T, U>> Zip(const LazySequence<U>& other) const {
        LazySequence<T> left = *this;
        LazySequence<U> right = other;
        Cardinal length = Cardinal::Min(left.GetLength(), right.GetLength());

        return LazySequence<Tuple<T, U>>::Create(
            length,
            [length]() { return length; },
            [left, right](size_t index, const Sequence<Tuple<T, U>>*) mutable -> Tuple<T, U> {
                return Tuple<T, U>(
                    left.Get(static_cast<int>(index)),
                    right.Get(static_cast<int>(index))
                );
            }
        );
    }

    Generator GetGenerator(size_t startPosition = 0) const {
        return Generator(*this, startPosition);
    }
};
