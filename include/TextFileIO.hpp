#pragma once
#include <string>

class TextFileReader {
private:
    void* handle;

public:
    TextFileReader();
    TextFileReader(const TextFileReader&) = delete;
    TextFileReader& operator=(const TextFileReader&) = delete;
    ~TextFileReader();

    bool Open(const std::string& path);
    void Close();
    bool IsOpen() const;
    bool ReadLine(std::string& line);
    bool Rewind();
};

class TextFileWriter {
private:
    void* handle;

public:
    TextFileWriter();
    TextFileWriter(const TextFileWriter&) = delete;
    TextFileWriter& operator=(const TextFileWriter&) = delete;
    ~TextFileWriter();

    bool Open(const std::string& path);
    void Close();
    bool IsOpen() const;
    bool WriteLine(const std::string& line);
};

bool TryReadWholeTextFile(const std::string& path, std::string& text);

#include "TextFileIO.tpp"
