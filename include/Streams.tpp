#pragma once

inline bool IsSimpleWhitespace(char symbol) {
    return symbol == ' ' || symbol == '\n' || symbol == '\r' || symbol == '\t';
}

inline std::string NormalizeTokenSeparators(std::string text) {
    for (char& symbol : text) {
        if (symbol == ',' || symbol == ';' || symbol == '\n' || symbol == '\r' || symbol == '\t') {
            symbol = ' ';
        }
    }
    return text;
}

inline std::string ReadWholeFile(const std::string& filePath) {
    std::string text;
    if (!TryReadWholeTextFile(filePath, text)) {
        throw InvalidArgument("cannot open file: " + filePath);
    }
    return text;
}

inline size_t UnlimitedStreamTransfer() {
    return static_cast<size_t>(-1);
}

template<class T>
ArraySequence<T> ParseTokenSequence(const std::string& text, const Deserializer<T>& deserializer) {
    ArraySequence<T> items;
    std::string normalized = NormalizeTokenSeparators(text);
    size_t index = 0;

    while (index < normalized.size()) {
        while (index < normalized.size() && normalized[index] == ' ') {
            index++;
        }
        if (index >= normalized.size()) {
            break;
        }

        size_t start = index;
        while (index < normalized.size() && normalized[index] != ' ') {
            index++;
        }

        items.Append(deserializer(normalized.substr(start, index - start)));
    }

    return items;
}

template<class T>
ArraySequence<T> ParseJsonArray(const std::string& text, const Deserializer<T>& deserializer) {
    size_t leftBracket = text.find('[');
    size_t rightBracket = text.rfind(']');
    if (leftBracket == std::string::npos || rightBracket == std::string::npos || leftBracket > rightBracket) {
        throw InputError("JSON array expected");
    }

    std::string body = text.substr(leftBracket + 1, rightBracket - leftBracket - 1);
    for (char& symbol : body) {
        if (symbol == ',' || IsSimpleWhitespace(symbol)) {
            symbol = ' ';
        }
    }

    ArraySequence<T> items;
    size_t index = 0;
    while (index < body.size()) {
        while (index < body.size() && body[index] == ' ') {
            index++;
        }
        if (index >= body.size()) {
            break;
        }

        size_t start = index;
        while (index < body.size() && body[index] != ' ') {
            index++;
        }

        std::string token = body.substr(start, index - start);
        if (token == "null") {
            continue;
        }
        items.Append(deserializer(token));
    }

    return items;
}

template<class T>
void ReadOnlyStream<T>::EnsureOpen() const {
    if (!open) {
        throw StreamClosed();
    }
}

template<class T>
ReadOnlyStream<T>::ReadOnlyStream() : open(false), position(0) {}

template<class T>
void ReadOnlyStream<T>::Open() {
    open = true;
    position = 0;
}

template<class T>
void ReadOnlyStream<T>::Close() {
    open = false;
}

template<class T>
Option<T> ReadOnlyStream<T>::TryRead() {
    try {
        return Option<T>::Some(Read());
    } catch (const Exception&) {
        return Option<T>::None();
    }
}

template<class T>
size_t ReadOnlyStream<T>::GetPosition() const {
    return position;
}

template<class T>
SequenceReadStream<T>::SequenceReadStream(const Sequence<T>& sequence) : source(&sequence) {}

template<class T>
bool SequenceReadStream<T>::IsEndOfStream() const {
    return this->position >= static_cast<size_t>(source->GetLength());
}

template<class T>
T SequenceReadStream<T>::Read() {
    this->EnsureOpen();
    if (IsEndOfStream()) {
        throw EndOfStream();
    }
    return source->Get(static_cast<int>(this->position++));
}

template<class T>
bool SequenceReadStream<T>::IsCanSeek() const {
    return true;
}

template<class T>
size_t SequenceReadStream<T>::Seek(size_t index) {
    if (index > static_cast<size_t>(source->GetLength())) {
        throw IndexOutOfRange(static_cast<int>(index), source->GetLength());
    }
    this->position = index;
    return this->position;
}

template<class T>
bool SequenceReadStream<T>::IsCanGoBack() const {
    return true;
}

template<class T>
LazySequenceReadStream<T>::LazySequenceReadStream(const LazySequence<T>& sequence) : source(sequence) {}

template<class T>
bool LazySequenceReadStream<T>::IsEndOfStream() const {
    Cardinal length = source.GetLength();
    return length.IsFinite() && this->position >= length.AsFinite();
}

template<class T>
T LazySequenceReadStream<T>::Read() {
    this->EnsureOpen();
    if (IsEndOfStream()) {
        throw EndOfStream();
    }
    return source.Get(static_cast<int>(this->position++));
}

template<class T>
bool LazySequenceReadStream<T>::IsCanSeek() const {
    return true;
}

template<class T>
size_t LazySequenceReadStream<T>::Seek(size_t index) {
    Cardinal length = source.GetLength();
    if (length.IsFinite() && index > length.AsFinite()) {
        throw IndexOutOfRange(static_cast<int>(index), static_cast<int>(length.AsFinite()));
    }
    this->position = index;
    return this->position;
}

template<class T>
bool LazySequenceReadStream<T>::IsCanGoBack() const {
    return true;
}

template<class T>
StringReadStream<T>::StringReadStream(const std::string& text, const Deserializer<T>& deserializer)
    : items(ParseTokenSequence<T>(text, deserializer)) {}

template<class T>
bool StringReadStream<T>::IsEndOfStream() const {
    return this->position >= static_cast<size_t>(items.GetLength());
}

template<class T>
T StringReadStream<T>::Read() {
    this->EnsureOpen();
    if (IsEndOfStream()) {
        throw EndOfStream();
    }
    return items.Get(static_cast<int>(this->position++));
}

template<class T>
bool StringReadStream<T>::IsCanSeek() const {
    return true;
}

template<class T>
size_t StringReadStream<T>::Seek(size_t index) {
    if (index > static_cast<size_t>(items.GetLength())) {
        throw IndexOutOfRange(static_cast<int>(index), items.GetLength());
    }
    this->position = index;
    return this->position;
}

template<class T>
bool StringReadStream<T>::IsCanGoBack() const {
    return true;
}

template<class T>
CsvReadStream<T>::CsvReadStream(const std::string& text, const Deserializer<T>& deserializer)
    : items(ParseTokenSequence<T>(text, deserializer)) {}

template<class T>
CsvReadStream<T> CsvReadStream<T>::FromFile(
    const std::string& filePath,
    const Deserializer<T>& deserializer
) {
    return CsvReadStream<T>(ReadWholeFile(filePath), deserializer);
}

template<class T>
bool CsvReadStream<T>::IsEndOfStream() const {
    return this->position >= static_cast<size_t>(items.GetLength());
}

template<class T>
T CsvReadStream<T>::Read() {
    this->EnsureOpen();
    if (IsEndOfStream()) {
        throw EndOfStream();
    }
    return items.Get(static_cast<int>(this->position++));
}

template<class T>
bool CsvReadStream<T>::IsCanSeek() const {
    return true;
}

template<class T>
size_t CsvReadStream<T>::Seek(size_t index) {
    if (index > static_cast<size_t>(items.GetLength())) {
        throw IndexOutOfRange(static_cast<int>(index), items.GetLength());
    }
    this->position = index;
    return this->position;
}

template<class T>
bool CsvReadStream<T>::IsCanGoBack() const {
    return true;
}

template<class T>
JsonArrayReadStream<T>::JsonArrayReadStream(const std::string& text, const Deserializer<T>& deserializer)
    : items(ParseJsonArray<T>(text, deserializer)) {}

template<class T>
JsonArrayReadStream<T> JsonArrayReadStream<T>::FromFile(
    const std::string& filePath,
    const Deserializer<T>& deserializer
) {
    return JsonArrayReadStream<T>(ReadWholeFile(filePath), deserializer);
}

template<class T>
bool JsonArrayReadStream<T>::IsEndOfStream() const {
    return this->position >= static_cast<size_t>(items.GetLength());
}

template<class T>
T JsonArrayReadStream<T>::Read() {
    this->EnsureOpen();
    if (IsEndOfStream()) {
        throw EndOfStream();
    }
    return items.Get(static_cast<int>(this->position++));
}

template<class T>
bool JsonArrayReadStream<T>::IsCanSeek() const {
    return true;
}

template<class T>
size_t JsonArrayReadStream<T>::Seek(size_t index) {
    if (index > static_cast<size_t>(items.GetLength())) {
        throw IndexOutOfRange(static_cast<int>(index), items.GetLength());
    }
    this->position = index;
    return this->position;
}

template<class T>
bool JsonArrayReadStream<T>::IsCanGoBack() const {
    return true;
}

template<class T>
void FileReadStream<T>::ResetItems() {
    delete items;
    items = new MutableArraySequence<T>();
}

template<class T>
FileReadStream<T>::FileReadStream(const std::string& path, const Deserializer<T>& parser)
    : filePath(path),
      deserializer(parser),
      items(new MutableArraySequence<T>()) {}

template<class T>
FileReadStream<T>::~FileReadStream() {
    delete items;
}

template<class T>
void FileReadStream<T>::Open() {
    TextFileReader reader;
    if (!reader.Open(filePath)) {
        throw InvalidArgument("cannot open file: " + filePath);
    }

    ResetItems();
    std::string line;
    while (reader.ReadLine(line)) {
        if (!line.empty()) {
            items->Append(deserializer(line));
        }
    }

    ReadOnlyStream<T>::Open();
}

template<class T>
void FileReadStream<T>::Close() {
    ReadOnlyStream<T>::Close();
}

template<class T>
bool FileReadStream<T>::IsEndOfStream() const {
    this->EnsureOpen();
    return this->position >= static_cast<size_t>(items->GetLength());
}

template<class T>
T FileReadStream<T>::Read() {
    this->EnsureOpen();
    if (IsEndOfStream()) {
        throw EndOfStream();
    }
    return items->Get(static_cast<int>(this->position++));
}

template<class T>
bool FileReadStream<T>::IsCanSeek() const {
    return true;
}

template<class T>
size_t FileReadStream<T>::Seek(size_t index) {
    this->EnsureOpen();
    if (index > static_cast<size_t>(items->GetLength())) {
        throw IndexOutOfRange(static_cast<int>(index), items->GetLength());
    }
    this->position = index;
    return this->position;
}

template<class T>
bool FileReadStream<T>::IsCanGoBack() const {
    return true;
}

template<class T>
void WriteOnlyStream<T>::EnsureOpen() const {
    if (!open) {
        throw StreamClosed();
    }
}

template<class T>
WriteOnlyStream<T>::WriteOnlyStream() : open(false), position(0) {}

template<class T>
void WriteOnlyStream<T>::Open() {
    open = true;
    position = 0;
}

template<class T>
void WriteOnlyStream<T>::Close() {
    open = false;
}

template<class T>
size_t WriteOnlyStream<T>::GetPosition() const {
    return position;
}

template<class T>
size_t MemoryWriteStream<T>::Write(const T& item) {
    this->EnsureOpen();
    items.Append(item);
    this->position++;
    return this->position;
}

template<class T>
const ArraySequence<T>& MemoryWriteStream<T>::GetItems() const {
    return items;
}

template<class T>
FileWriteStream<T>::FileWriteStream(const std::string& path, const Serializer<T>& formatter)
    : filePath(path), serializer(formatter), file() {}

template<class T>
void FileWriteStream<T>::Open() {
    file.Close();
    if (!file.Open(filePath)) {
        throw InvalidArgument("cannot open file for writing: " + filePath);
    }
    WriteOnlyStream<T>::Open();
}

template<class T>
void FileWriteStream<T>::Close() {
    file.Close();
    WriteOnlyStream<T>::Close();
}

template<class T>
size_t FileWriteStream<T>::Write(const T& item) {
    this->EnsureOpen();
    if (!file.WriteLine(serializer(item))) {
        throw InvalidState("failed to write to file: " + filePath);
    }
    this->position++;
    return this->position;
}

template<class T>
size_t Pump(ReadOnlyStream<T>& reader, WriteOnlyStream<T>& writer, size_t limit) {
    size_t transferred = 0;
    while (transferred < limit && !reader.IsEndOfStream()) {
        writer.Write(reader.Read());
        transferred++;
    }
    return transferred;
}
