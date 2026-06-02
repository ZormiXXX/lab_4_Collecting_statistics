#pragma once
#include <functional>
#include "ArraySequence.hpp"
#include "Cardinal.hpp"
#include "Mutable/MutableArraySequence.hpp"

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
        explicit Generator(const LazySequence<T>& sequence, size_t startPosition = 0);

        bool HasNext() const;
        T GetNext();
        Option<T> TryGetNext();
    };

private:
    struct State {
        size_t refCount;
        Cardinal lengthHint;
        Cardinal exactLength;
        bool exactLengthKnown;
        bool finished;
        LengthResolver lengthResolver;
        NextFunction nextFunction;
        MutableArraySequence<T> materialized;
        LazySequence<T>* omegaTail;

        State();
        ~State();
    };

    State* state;

    explicit LazySequence(State* sharedState);

    static State* AllocateState();
    static void RetainState(State* sharedState);
    static void ReleaseState(State* sharedState);

    static LazySequence<T> Create(
        const Cardinal& lengthHint,
        const LengthResolver& lengthResolver,
        const NextFunction& nextFunction,
        bool finished = false,
        bool exactLengthKnown = true
    );

    static LazySequence<T> CreateWithTail(
        const Cardinal& lengthHint,
        const LengthResolver& lengthResolver,
        const NextFunction& nextFunction,
        const LazySequence<T>* omegaTail,
        bool finished = false,
        bool exactLengthKnown = true
    );

    void EnsureCanIndex(int index) const;
    void EnsureMaterialized(size_t index) const;
    void MaterializeAllFinite() const;
    bool HasOmegaTail() const;
    LazySequence<T> GetOmegaTail() const;

public:
    LazySequence();
    LazySequence(const T* items, int count);
    explicit LazySequence(const Sequence<T>* sequence);
    LazySequence(const NextFunction& recurrenceRule, const Sequence<T>* seed);
    LazySequence(const LazySequence<T>& other);
    LazySequence(LazySequence<T>&& other) noexcept;
    LazySequence<T>& operator=(const LazySequence<T>& other);
    LazySequence<T>& operator=(LazySequence<T>&& other) noexcept;
    ~LazySequence();

    static LazySequence<T> GenerateIndexed(
        const std::function<T(size_t)>& generator,
        const Cardinal& lengthHint = Cardinal::Infinite()
    );

    T Get(int index) const;
    T Get(const Ordinal& index) const;
    T GetFirst() const;
    T GetLast() const;
    Cardinal GetLength() const;
    size_t GetMaterializedCount() const;
    bool IsInfinite() const;
    Option<T> TryGet(const Ordinal& index) const;
    ArraySequence<T>* ToSequence(size_t limit = 0, bool useLimit = false) const;

    LazySequence<T> GetSubsequence(int startIndex, int endIndex) const;
    LazySequence<T> Take(size_t count) const;
    LazySequence<T> Prepend(const T& item) const;
    LazySequence<T> Concat(const LazySequence<T>& other) const;
    LazySequence<T> Append(const T& item) const;
    LazySequence<T> InsertAt(const T& item, int index) const;

    template<class U, class Mapper>
    LazySequence<U> Map(Mapper mapper) const;

    template<class Reducer, class U>
    U Reduce(Reducer reducer, U initial) const;

    template<class Reducer, class U>
    U ReduceFirst(Reducer reducer, U initial, size_t count) const;

    template<class Predicate>
    LazySequence<T> Where(Predicate predicate) const;

    template<class U>
    LazySequence<Tuple<T, U>> Zip(const Sequence<U>& other) const;

    template<class U>
    LazySequence<Tuple<T, U>> Zip(const LazySequence<U>& other) const;

    Generator GetGenerator(size_t startPosition = 0) const;
};

#include "LazySequence.tpp"
