#pragma once

template<class T>
LazySequence<T>::Generator::Generator(const LazySequence<T>& sequence, size_t startPosition)
    : owner(&sequence), position(startPosition) {}

template<class T>
bool LazySequence<T>::Generator::HasNext() const {
    Cardinal length = owner->GetLength();
    return length.IsInfinite() || position < length.AsFinite();
}

template<class T>
T LazySequence<T>::Generator::GetNext() {
    T value = owner->Get(static_cast<int>(position));
    position++;
    return value;
}

template<class T>
Option<T> LazySequence<T>::Generator::TryGetNext() {
    try {
        return Option<T>::Some(GetNext());
    } catch (const Exception&) {
        return Option<T>::None();
    }
}

template<class T>
LazySequence<T>::State::State()
    : refCount(1),
      lengthHint(Cardinal::Finite(0)),
      exactLength(Cardinal::Finite(0)),
      exactLengthKnown(true),
      finished(true),
      lengthResolver(),
      nextFunction(),
      materialized(),
      omegaTail(nullptr) {}

template<class T>
LazySequence<T>::State::~State() {
    delete omegaTail;
}

template<class T>
LazySequence<T>::LazySequence(State* sharedState) : state(sharedState) {}

template<class T>
typename LazySequence<T>::State* LazySequence<T>::AllocateState() {
    return new State();
}

template<class T>
void LazySequence<T>::RetainState(State* sharedState) {
    if (sharedState != nullptr) {
        sharedState->refCount++;
    }
}

template<class T>
void LazySequence<T>::ReleaseState(State* sharedState) {
    if (sharedState == nullptr) {
        return;
    }

    if (sharedState->refCount > 1) {
        sharedState->refCount--;
        return;
    }

    delete sharedState;
}

template<class T>
LazySequence<T> LazySequence<T>::Create(
    const Cardinal& lengthHint,
    const LengthResolver& lengthResolver,
    const NextFunction& nextFunction,
    bool finished,
    bool exactLengthKnown
) {
    return CreateWithTail(
        lengthHint,
        lengthResolver,
        nextFunction,
        nullptr,
        finished,
        exactLengthKnown
    );
}

template<class T>
LazySequence<T> LazySequence<T>::CreateWithTail(
    const Cardinal& lengthHint,
    const LengthResolver& lengthResolver,
    const NextFunction& nextFunction,
    const LazySequence<T>* omegaTail,
    bool finished,
    bool exactLengthKnown
) {
    State* sharedState = AllocateState();
    sharedState->lengthHint = lengthHint;
    sharedState->exactLength = lengthHint;
    sharedState->exactLengthKnown = exactLengthKnown;
    sharedState->finished = finished;
    sharedState->lengthResolver = lengthResolver;
    sharedState->nextFunction = nextFunction;
    sharedState->omegaTail = omegaTail != nullptr ? new LazySequence<T>(*omegaTail) : nullptr;
    return LazySequence<T>(sharedState);
}

template<class T>
void LazySequence<T>::EnsureCanIndex(int index) const {
    if (index < 0) {
        throw IndexOutOfRange(index, state->materialized.GetLength());
    }
}

template<class T>
void LazySequence<T>::EnsureMaterialized(size_t index) const {
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

template<class T>
void LazySequence<T>::MaterializeAllFinite() const {
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

template<class T>
bool LazySequence<T>::HasOmegaTail() const {
    return state != nullptr && state->omegaTail != nullptr;
}

template<class T>
LazySequence<T> LazySequence<T>::GetOmegaTail() const {
    return *state->omegaTail;
}

template<class T>
LazySequence<T>::LazySequence() : state(AllocateState()) {}

template<class T>
LazySequence<T>::LazySequence(const T* items, int count) : state(AllocateState()) {
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

template<class T>
LazySequence<T>::LazySequence(const Sequence<T>* sequence) : state(AllocateState()) {
    if (sequence == nullptr) {
        throw InvalidArgument("sequence must not be null");
    }

    int length = sequence->GetLength();
    state->lengthHint = Cardinal::Finite(length);
    state->exactLength = Cardinal::Finite(length);
    state->exactLengthKnown = true;
    state->finished = false;

    MutableArraySequence<T> source;
    for (int index = 0; index < length; index++) {
        source.Append(sequence->Get(index));
    }

    state->nextFunction = [source](size_t index, const Sequence<T>*) -> T {
        return source.Get(static_cast<int>(index));
    };
}

template<class T>
LazySequence<T>::LazySequence(const NextFunction& recurrenceRule, const Sequence<T>* seed)
    : state(AllocateState()) {
    if (seed == nullptr) {
        throw InvalidArgument("seed must not be null");
    }

    int seedLength = seed->GetLength();
    if (seedLength < 0) {
        throw InvalidArgument("seed length must be non-negative");
    }

    MutableArraySequence<T> seedValues;
    for (int index = 0; index < seedLength; index++) {
        seedValues.Append(seed->Get(index));
    }

    state->lengthHint = Cardinal::Infinite();
    state->exactLengthKnown = false;
    state->finished = false;
    state->nextFunction = [seedValues, recurrenceRule](size_t index, const Sequence<T>* materialized) -> T {
        if (index < static_cast<size_t>(seedValues.GetLength())) {
            return seedValues.Get(static_cast<int>(index));
        }
        return recurrenceRule(index, materialized);
    };
}

template<class T>
LazySequence<T>::LazySequence(const LazySequence<T>& other) : state(other.state) {
    RetainState(state);
}

template<class T>
LazySequence<T>::LazySequence(LazySequence<T>&& other) noexcept : state(other.state) {
    other.state = nullptr;
}

template<class T>
LazySequence<T>& LazySequence<T>::operator=(const LazySequence<T>& other) {
    if (this != &other) {
        RetainState(other.state);
        ReleaseState(state);
        state = other.state;
    }
    return *this;
}

template<class T>
LazySequence<T>& LazySequence<T>::operator=(LazySequence<T>&& other) noexcept {
    if (this != &other) {
        ReleaseState(state);
        state = other.state;
        other.state = nullptr;
    }
    return *this;
}

template<class T>
LazySequence<T>::~LazySequence() {
    ReleaseState(state);
}

template<class T>
LazySequence<T> LazySequence<T>::GenerateIndexed(
    const std::function<T(size_t)>& generator,
    const Cardinal& lengthHint
) {
    return Create(
        lengthHint,
        LengthResolver(),
        [generator](size_t index, const Sequence<T>*) -> T {
            return generator(index);
        }
    );
}

template<class T>
T LazySequence<T>::Get(int index) const {
    EnsureCanIndex(index);
    EnsureMaterialized(static_cast<size_t>(index));
    return state->materialized.Get(index);
}

template<class T>
T LazySequence<T>::Get(const Ordinal& index) const {
    if (index.IsFinite()) {
        return Get(static_cast<int>(index.AsFinite()));
    }

    if (!HasOmegaTail()) {
        throw IndexOutOfRange(0, state->materialized.GetLength());
    }

    LazySequence<T> omegaTail = GetOmegaTail();
    return omegaTail.Get(static_cast<int>(index.GetOffset()));
}

template<class T>
T LazySequence<T>::GetFirst() const {
    return Get(0);
}

template<class T>
T LazySequence<T>::GetLast() const {
    if (HasOmegaTail()) {
        LazySequence<T> omegaTail = GetOmegaTail();
        Cardinal tailLength = omegaTail.GetLength();
        if (tailLength.IsFinite() && tailLength.AsFinite() > 0) {
            return omegaTail.GetLast();
        }
    }

    Cardinal length = GetLength();
    if (length.IsInfinite()) {
        throw InfinityError("cannot get the last element of an infinite lazy sequence");
    }
    if (length.AsFinite() == 0) {
        throw IndexOutOfRange(0, 0);
    }
    return Get(static_cast<int>(length.AsFinite() - 1));
}

template<class T>
Cardinal LazySequence<T>::GetLength() const {
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

template<class T>
size_t LazySequence<T>::GetMaterializedCount() const {
    return static_cast<size_t>(state->materialized.GetLength());
}

template<class T>
bool LazySequence<T>::IsInfinite() const {
    return GetLength().IsInfinite();
}

template<class T>
Option<T> LazySequence<T>::TryGet(const Ordinal& index) const {
    try {
        return Option<T>::Some(Get(index));
    } catch (const Exception&) {
        return Option<T>::None();
    }
}

template<class T>
ArraySequence<T>* LazySequence<T>::ToSequence(size_t limit, bool useLimit) const {
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

template<class T>
LazySequence<T> LazySequence<T>::GetSubsequence(int startIndex, int endIndex) const {
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
        [source, start](size_t index, const Sequence<T>*) -> T {
            return source.Get(static_cast<int>(start + index));
        }
    );
}

template<class T>
LazySequence<T> LazySequence<T>::Take(size_t count) const {
    if (count == 0) {
        return LazySequence<T>();
    }
    return GetSubsequence(0, static_cast<int>(count - 1));
}

template<class T>
LazySequence<T> LazySequence<T>::Prepend(const T& item) const {
    LazySequence<T> source = *this;
    Cardinal sourceLength = source.GetLength();
    Cardinal newLength = sourceLength.IsInfinite() ? Cardinal::Infinite() : sourceLength.Add(1);
    if (source.HasOmegaTail()) {
        LazySequence<T> omegaTail = source.GetOmegaTail();
        return CreateWithTail(
            newLength,
            [newLength]() { return newLength; },
            [source, item](size_t index, const Sequence<T>*) -> T {
                return index == 0 ? item : source.Get(static_cast<int>(index - 1));
            },
            &omegaTail
        );
    }

    return Create(
        newLength,
        [newLength]() { return newLength; },
        [source, item](size_t index, const Sequence<T>*) -> T {
            return index == 0 ? item : source.Get(static_cast<int>(index - 1));
        }
    );
}

template<class T>
LazySequence<T> LazySequence<T>::Concat(const LazySequence<T>& other) const {
    LazySequence<T> left = *this;
    LazySequence<T> right = other;
    Cardinal leftLength = left.GetLength();

    if (leftLength.IsInfinite()) {
        if (left.HasOmegaTail()) {
            LazySequence<T> leftTail = left.GetOmegaTail();
            LazySequence<T> mergedTail = leftTail.Concat(right);
            return CreateWithTail(
                Cardinal::Infinite(),
                []() { return Cardinal::Infinite(); },
                [left](size_t index, const Sequence<T>*) -> T {
                    return left.Get(static_cast<int>(index));
                },
                &mergedTail
            );
        }

        return CreateWithTail(
            Cardinal::Infinite(),
            []() { return Cardinal::Infinite(); },
            [left](size_t index, const Sequence<T>*) -> T {
                return left.Get(static_cast<int>(index));
            },
            &right
        );
    }

    Cardinal rightLength = right.GetLength();
    Cardinal totalLength = Cardinal::Add(leftLength, rightLength);
    size_t border = leftLength.AsFinite();

    if (right.HasOmegaTail()) {
        LazySequence<T> omegaTail = right.GetOmegaTail();
        return CreateWithTail(
            totalLength,
            [totalLength]() { return totalLength; },
            [left, right, border](size_t index, const Sequence<T>*) -> T {
                if (index < border) {
                    return left.Get(static_cast<int>(index));
                }
                return right.Get(static_cast<int>(index - border));
            },
            &omegaTail
        );
    }

    return Create(
        totalLength,
        [totalLength]() { return totalLength; },
        [left, right, border](size_t index, const Sequence<T>*) -> T {
            if (index < border) {
                return left.Get(static_cast<int>(index));
            }
            return right.Get(static_cast<int>(index - border));
        }
    );
}

template<class T>
LazySequence<T> LazySequence<T>::Append(const T& item) const {
    T singleItem[1] = {item};
    LazySequence<T> tail(singleItem, 1);
    return Concat(tail);
}

template<class T>
LazySequence<T> LazySequence<T>::InsertAt(const T& item, int index) const {
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

    if (source.HasOmegaTail()) {
        LazySequence<T> omegaTail = source.GetOmegaTail();
        return CreateWithTail(
            newLength,
            [newLength]() { return newLength; },
            [source, item, insertIndex](size_t current, const Sequence<T>*) -> T {
                if (current == insertIndex) {
                    return item;
                }
                if (current < insertIndex) {
                    return source.Get(static_cast<int>(current));
                }
                return source.Get(static_cast<int>(current - 1));
            },
            &omegaTail
        );
    }

    return Create(
        newLength,
        [newLength]() { return newLength; },
        [source, item, insertIndex](size_t current, const Sequence<T>*) -> T {
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

template<class T>
template<class U, class Mapper>
LazySequence<U> LazySequence<T>::Map(Mapper mapper) const {
    LazySequence<T> source = *this;
    Cardinal length = source.GetLength();
    if (source.HasOmegaTail()) {
        LazySequence<T> omegaTail = source.GetOmegaTail();
        LazySequence<U> mappedTail = omegaTail.template Map<U>(mapper);
        return LazySequence<U>::CreateWithTail(
            length,
            [length]() { return length; },
            [source, mapper](size_t index, const Sequence<U>*) -> U {
                return mapper(source.Get(static_cast<int>(index)));
            },
            &mappedTail
        );
    }

    return LazySequence<U>::Create(
        length,
        [length]() { return length; },
        [source, mapper](size_t index, const Sequence<U>*) -> U {
            return mapper(source.Get(static_cast<int>(index)));
        }
    );
}

template<class T>
template<class Reducer, class U>
U LazySequence<T>::Reduce(Reducer reducer, U initial) const {
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

template<class T>
template<class Reducer, class U>
U LazySequence<T>::ReduceFirst(Reducer reducer, U initial, size_t count) const {
    U result = initial;
    for (size_t index = 0; index < count; index++) {
        result = reducer(result, Get(static_cast<int>(index)));
    }
    return result;
}

template<class T>
template<class Predicate>
LazySequence<T> LazySequence<T>::Where(Predicate predicate) const {
    LazySequence<T> source = *this;
    Cardinal sourceLength = source.GetLength();
    size_t nextSourceIndex = 0;
    if (source.HasOmegaTail()) {
        LazySequence<T> omegaTail = source.GetOmegaTail();
        LazySequence<T> filteredTail = omegaTail.Where(predicate);
        return CreateWithTail(
            sourceLength.IsInfinite() ? Cardinal::Infinite() : Cardinal::Finite(0),
            [source, predicate, sourceLength]() -> Cardinal {
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
                    T candidate = source.Get(static_cast<int>(nextSourceIndex));
                    nextSourceIndex++;
                    if (predicate(candidate)) {
                        return candidate;
                    }
                }
            },
            &filteredTail,
            false,
            sourceLength.IsInfinite()
        );
    }

    return Create(
        sourceLength.IsInfinite() ? Cardinal::Infinite() : Cardinal::Finite(0),
        [source, predicate, sourceLength]() -> Cardinal {
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
                T candidate = source.Get(static_cast<int>(nextSourceIndex));
                nextSourceIndex++;
                if (predicate(candidate)) {
                    return candidate;
                }
            }
        },
        false,
        sourceLength.IsInfinite()
    );
}

template<class T>
template<class U>
LazySequence<Tuple<T, U>> LazySequence<T>::Zip(const Sequence<U>& other) const {
    LazySequence<T> source = *this;
    Cardinal length = Cardinal::Min(source.GetLength(), Cardinal::Finite(other.GetLength()));
    MutableArraySequence<U> copied;
    for (int index = 0; index < other.GetLength(); index++) {
        copied.Append(other.Get(index));
    }

    return LazySequence<Tuple<T, U>>::Create(
        length,
        [length]() { return length; },
        [source, copied](size_t index, const Sequence<Tuple<T, U>>*) -> Tuple<T, U> {
            return Tuple<T, U>(
                source.Get(static_cast<int>(index)),
                copied.Get(static_cast<int>(index))
            );
        }
    );
}

template<class T>
template<class U>
LazySequence<Tuple<T, U>> LazySequence<T>::Zip(const LazySequence<U>& other) const {
    LazySequence<T> left = *this;
    LazySequence<U> right = other;
    Cardinal length = Cardinal::Min(left.GetLength(), right.GetLength());

    return LazySequence<Tuple<T, U>>::Create(
        length,
        [length]() { return length; },
        [left, right](size_t index, const Sequence<Tuple<T, U>>*) -> Tuple<T, U> {
            return Tuple<T, U>(
                left.Get(static_cast<int>(index)),
                right.Get(static_cast<int>(index))
            );
        }
    );
}

template<class T>
typename LazySequence<T>::Generator LazySequence<T>::GetGenerator(size_t startPosition) const {
    return Generator(*this, startPosition);
}
