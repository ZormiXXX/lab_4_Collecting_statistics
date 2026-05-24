#pragma once

template<class T>
class IEnumerator {
public:
    virtual ~IEnumerator() = default;
    virtual bool MoveNext() = 0;
    virtual const T& GetCurrent() const = 0;
    virtual void Reset() = 0;
};
