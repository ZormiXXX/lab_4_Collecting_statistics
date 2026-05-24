#pragma once
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include "LazySequence.hpp"

template<class T>
using Serializer = std::function<std::string(const T&)>;

template<class T>
using Deserializer = std::function<T(const std::string&)>;

inline std::string NormalizeTokenSeparators(std::string text) {
    for (char& symbol : text) {
        if (symbol == ',' || symbol == ';' || symbol == '\n' || symbol == '\t') {
            symbol = ' ';
        }
    }
    return text;
}

template<class T>
class ReadOnlyStream {
protected:
    bool open;
    size_t position;

    void EnsureOpen() const {
        if (!open) {
            throw StreamClosed();
        }
    }

public:
    ReadOnlyStream() : open(false), position(0) {}

    virtual ~ReadOnlyStream() = default;

    virtual void Open() {
        open = true;
        position = 0;
    }

    virtual void Close() {
        open = false;
    }

    virtual bool IsEndOfStream() const = 0;
    virtual T Read() = 0;

    size_t GetPosition() const {
        return position;
    }

    virtual bool IsCanSeek() const = 0;
    virtual size_t Seek(size_t index) = 0;
    virtual bool IsCanGoBack() const = 0;
};

template<class T>
class SequenceReadStream : public ReadOnlyStream<T> {
private:
    const Sequence<T>* source;

public:
    explicit SequenceReadStream(const Sequence<T>& sequence) : source(&sequence) {}

    bool IsEndOfStream() const override {
        return this->position >= static_cast<size_t>(source->GetLength());
    }

    T Read() override {
        this->EnsureOpen();
        if (IsEndOfStream()) {
            throw EndOfStream();
        }
        return source->Get(static_cast<int>(this->position++));
    }

    bool IsCanSeek() const override {
        return true;
    }

    size_t Seek(size_t index) override {
        if (index > static_cast<size_t>(source->GetLength())) {
            throw IndexOutOfRange(static_cast<int>(index), source->GetLength());
        }
        this->position = index;
        return this->position;
    }

    bool IsCanGoBack() const override {
        return true;
    }
};

template<class T>
class LazySequenceReadStream : public ReadOnlyStream<T> {
private:
    LazySequence<T> source;

public:
    explicit LazySequenceReadStream(const LazySequence<T>& sequence) : source(sequence) {}

    bool IsEndOfStream() const override {
        Cardinal length = source.GetLength();
        return length.IsFinite() && this->position >= length.AsFinite();
    }

    T Read() override {
        this->EnsureOpen();
        if (IsEndOfStream()) {
            throw EndOfStream();
        }
        return source.Get(static_cast<int>(this->position++));
    }

    bool IsCanSeek() const override {
        return true;
    }

    size_t Seek(size_t index) override {
        Cardinal length = source.GetLength();
        if (length.IsFinite() && index > length.AsFinite()) {
            throw IndexOutOfRange(static_cast<int>(index), static_cast<int>(length.AsFinite()));
        }
        this->position = index;
        return this->position;
    }

    bool IsCanGoBack() const override {
        return true;
    }
};

template<class T>
class StringReadStream : public ReadOnlyStream<T> {
private:
    ArraySequence<T> items;

public:
    StringReadStream(const std::string& text, const Deserializer<T>& deserializer) : items() {
        std::istringstream stream(NormalizeTokenSeparators(text));
        std::string token;
        while (stream >> token) {
            items.Append(deserializer(token));
        }
    }

    bool IsEndOfStream() const override {
        return this->position >= static_cast<size_t>(items.GetLength());
    }

    T Read() override {
        this->EnsureOpen();
        if (IsEndOfStream()) {
            throw EndOfStream();
        }
        return items.Get(static_cast<int>(this->position++));
    }

    bool IsCanSeek() const override {
        return true;
    }

    size_t Seek(size_t index) override {
        if (index > static_cast<size_t>(items.GetLength())) {
            throw IndexOutOfRange(static_cast<int>(index), items.GetLength());
        }
        this->position = index;
        return this->position;
    }

    bool IsCanGoBack() const override {
        return true;
    }
};

template<class T>
class FileReadStream : public ReadOnlyStream<T> {
private:
    std::string filePath;
    Deserializer<T> deserializer;
    mutable std::ifstream file;
    mutable bool prefetched;
    mutable bool hasPrefetchedValue;
    mutable std::string prefetchedLine;

    void ResetPrefetch() const {
        prefetched = false;
        hasPrefetchedValue = false;
        prefetchedLine.clear();
    }

    void Prefetch() const {
        if (prefetched) {
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                prefetchedLine = line;
                prefetched = true;
                hasPrefetchedValue = true;
                return;
            }
        }

        prefetched = true;
        hasPrefetchedValue = false;
        prefetchedLine.clear();
    }

public:
    FileReadStream(const std::string& path, const Deserializer<T>& parser)
        : filePath(path),
          deserializer(parser),
          file(),
          prefetched(false),
          hasPrefetchedValue(false),
          prefetchedLine() {}

    void Open() override {
        file.close();
        file.clear();
        file.open(filePath);
        if (!file.is_open()) {
            throw InvalidArgument("cannot open file: " + filePath);
        }
        ReadOnlyStream<T>::Open();
        ResetPrefetch();
    }

    void Close() override {
        file.close();
        ResetPrefetch();
        ReadOnlyStream<T>::Close();
    }

    bool IsEndOfStream() const override {
        this->EnsureOpen();
        Prefetch();
        return !hasPrefetchedValue;
    }

    T Read() override {
        this->EnsureOpen();
        Prefetch();
        if (!hasPrefetchedValue) {
            throw EndOfStream();
        }

        std::string line = prefetchedLine;
        ResetPrefetch();
        this->position++;
        return deserializer(line);
    }

    bool IsCanSeek() const override {
        return true;
    }

    size_t Seek(size_t index) override {
        this->EnsureOpen();

        file.clear();
        file.seekg(0);
        if (!file.good()) {
            throw InvalidArgument("cannot seek in file: " + filePath);
        }

        ResetPrefetch();
        this->position = 0;
        for (size_t skipped = 0; skipped < index; skipped++) {
            std::string line;
            if (!std::getline(file, line)) {
                throw IndexOutOfRange(static_cast<int>(index), static_cast<int>(this->position));
            }
            if (!line.empty()) {
                this->position++;
            } else {
                skipped--;
            }
        }
        return this->position;
    }

    bool IsCanGoBack() const override {
        return true;
    }
};

template<class T>
class WriteOnlyStream {
protected:
    bool open;
    size_t position;

    void EnsureOpen() const {
        if (!open) {
            throw StreamClosed();
        }
    }

public:
    WriteOnlyStream() : open(false), position(0) {}

    virtual ~WriteOnlyStream() = default;

    virtual void Open() {
        open = true;
        position = 0;
    }

    virtual void Close() {
        open = false;
    }

    size_t GetPosition() const {
        return position;
    }

    virtual size_t Write(const T& item) = 0;
};

template<class T>
class MemoryWriteStream : public WriteOnlyStream<T> {
private:
    ArraySequence<T> items;

public:
    size_t Write(const T& item) override {
        this->EnsureOpen();
        items.Append(item);
        this->position++;
        return this->position;
    }

    const ArraySequence<T>& GetItems() const {
        return items;
    }
};

template<class T>
class FileWriteStream : public WriteOnlyStream<T> {
private:
    std::string filePath;
    Serializer<T> serializer;
    std::ofstream file;

public:
    FileWriteStream(const std::string& path, const Serializer<T>& formatter)
        : filePath(path), serializer(formatter), file() {}

    void Open() override {
        file.close();
        file.clear();
        file.open(filePath, std::ios::trunc);
        if (!file.is_open()) {
            throw InvalidArgument("cannot open file for writing: " + filePath);
        }
        WriteOnlyStream<T>::Open();
    }

    void Close() override {
        file.close();
        WriteOnlyStream<T>::Close();
    }

    size_t Write(const T& item) override {
        this->EnsureOpen();
        file << serializer(item) << "\n";
        if (!file.good()) {
            throw InvalidState("failed to write to file: " + filePath);
        }
        this->position++;
        return this->position;
    }
};

template<class T>
size_t Pump(ReadOnlyStream<T>& reader, WriteOnlyStream<T>& writer, size_t limit = std::numeric_limits<size_t>::max()) {
    size_t transferred = 0;
    while (transferred < limit && !reader.IsEndOfStream()) {
        writer.Write(reader.Read());
        transferred++;
    }
    return transferred;
}
