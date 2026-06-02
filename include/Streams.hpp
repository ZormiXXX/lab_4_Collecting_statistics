#pragma once
#include <string>
#include "LazySequence.hpp"
#include "Mutable/MutableArraySequence.hpp"
#include "TextFileIO.hpp"

template<class T>
using Serializer = std::function<std::string(const T&)>;

template<class T>
using Deserializer = std::function<T(const std::string&)>;

inline std::string NormalizeTokenSeparators(std::string text);
inline std::string ReadWholeFile(const std::string& filePath);
inline size_t UnlimitedStreamTransfer();

template<class T>
ArraySequence<T> ParseTokenSequence(const std::string& text, const Deserializer<T>& deserializer);

template<class T>
ArraySequence<T> ParseJsonArray(const std::string& text, const Deserializer<T>& deserializer);

template<class T>
class ReadOnlyStream {
protected:
    bool open;
    size_t position;

    void EnsureOpen() const;

public:
    ReadOnlyStream();
    virtual ~ReadOnlyStream() = default;

    virtual void Open();
    virtual void Close();

    virtual bool IsEndOfStream() const = 0;
    virtual T Read() = 0;

    Option<T> TryRead();
    size_t GetPosition() const;

    virtual bool IsCanSeek() const = 0;
    virtual size_t Seek(size_t index) = 0;
    virtual bool IsCanGoBack() const = 0;
};

template<class T>
class SequenceReadStream : public ReadOnlyStream<T> {
private:
    const Sequence<T>* source;

public:
    explicit SequenceReadStream(const Sequence<T>& sequence);

    bool IsEndOfStream() const override;
    T Read() override;
    bool IsCanSeek() const override;
    size_t Seek(size_t index) override;
    bool IsCanGoBack() const override;
};

template<class T>
class LazySequenceReadStream : public ReadOnlyStream<T> {
private:
    LazySequence<T> source;

public:
    explicit LazySequenceReadStream(const LazySequence<T>& sequence);

    bool IsEndOfStream() const override;
    T Read() override;
    bool IsCanSeek() const override;
    size_t Seek(size_t index) override;
    bool IsCanGoBack() const override;
};

template<class T>
class StringReadStream : public ReadOnlyStream<T> {
private:
    ArraySequence<T> items;

public:
    StringReadStream(const std::string& text, const Deserializer<T>& deserializer);

    bool IsEndOfStream() const override;
    T Read() override;
    bool IsCanSeek() const override;
    size_t Seek(size_t index) override;
    bool IsCanGoBack() const override;
};

template<class T>
class CsvReadStream : public ReadOnlyStream<T> {
private:
    ArraySequence<T> items;

public:
    CsvReadStream(const std::string& text, const Deserializer<T>& deserializer);

    static CsvReadStream<T> FromFile(const std::string& filePath, const Deserializer<T>& deserializer);

    bool IsEndOfStream() const override;
    T Read() override;
    bool IsCanSeek() const override;
    size_t Seek(size_t index) override;
    bool IsCanGoBack() const override;
};

template<class T>
class JsonArrayReadStream : public ReadOnlyStream<T> {
private:
    ArraySequence<T> items;

public:
    JsonArrayReadStream(const std::string& text, const Deserializer<T>& deserializer);

    static JsonArrayReadStream<T> FromFile(
        const std::string& filePath,
        const Deserializer<T>& deserializer
    );

    bool IsEndOfStream() const override;
    T Read() override;
    bool IsCanSeek() const override;
    size_t Seek(size_t index) override;
    bool IsCanGoBack() const override;
};

template<class T>
class FileReadStream : public ReadOnlyStream<T> {
private:
    std::string filePath;
    Deserializer<T> deserializer;
    MutableArraySequence<T>* items;

    void ResetItems();

public:
    FileReadStream(const std::string& path, const Deserializer<T>& parser);
    ~FileReadStream() override;

    void Open() override;
    void Close() override;
    bool IsEndOfStream() const override;
    T Read() override;
    bool IsCanSeek() const override;
    size_t Seek(size_t index) override;
    bool IsCanGoBack() const override;
};

template<class T>
class WriteOnlyStream {
protected:
    bool open;
    size_t position;

    void EnsureOpen() const;

public:
    WriteOnlyStream();
    virtual ~WriteOnlyStream() = default;

    virtual void Open();
    virtual void Close();

    size_t GetPosition() const;
    virtual size_t Write(const T& item) = 0;
};

template<class T>
class MemoryWriteStream : public WriteOnlyStream<T> {
private:
    ArraySequence<T> items;

public:
    size_t Write(const T& item) override;
    const ArraySequence<T>& GetItems() const;
};

template<class T>
class FileWriteStream : public WriteOnlyStream<T> {
private:
    std::string filePath;
    Serializer<T> serializer;
    TextFileWriter file;

public:
    FileWriteStream(const std::string& path, const Serializer<T>& formatter);

    void Open() override;
    void Close() override;
    size_t Write(const T& item) override;
};

template<class T>
size_t Pump(
    ReadOnlyStream<T>& reader,
    WriteOnlyStream<T>& writer,
    size_t limit = UnlimitedStreamTransfer()
);

#include "Streams.tpp"
