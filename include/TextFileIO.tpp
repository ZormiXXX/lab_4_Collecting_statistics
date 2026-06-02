#pragma once
#include <cstdio>

inline std::FILE* AsTextFileHandle(void* handle) {
    return static_cast<std::FILE*>(handle);
}

inline TextFileReader::TextFileReader() : handle(nullptr) {}

inline TextFileReader::~TextFileReader() {
    Close();
}

inline bool TextFileReader::Open(const std::string& path) {
    Close();
    handle = std::fopen(path.c_str(), "r");
    return handle != nullptr;
}

inline void TextFileReader::Close() {
    if (handle != nullptr) {
        std::fclose(AsTextFileHandle(handle));
        handle = nullptr;
    }
}

inline bool TextFileReader::IsOpen() const {
    return handle != nullptr;
}

inline bool TextFileReader::ReadLine(std::string& line) {
    line.clear();
    if (handle == nullptr) {
        return false;
    }

    std::FILE* file = AsTextFileHandle(handle);
    int current = std::fgetc(file);
    if (current == EOF) {
        return false;
    }

    while (current != EOF && current != '\n') {
        if (current != '\r') {
            line += static_cast<char>(current);
        }
        current = std::fgetc(file);
    }
    return true;
}

inline bool TextFileReader::Rewind() {
    if (handle == nullptr) {
        return false;
    }

    std::clearerr(AsTextFileHandle(handle));
    std::rewind(AsTextFileHandle(handle));
    return true;
}

inline TextFileWriter::TextFileWriter() : handle(nullptr) {}

inline TextFileWriter::~TextFileWriter() {
    Close();
}

inline bool TextFileWriter::Open(const std::string& path) {
    Close();
    handle = std::fopen(path.c_str(), "w");
    return handle != nullptr;
}

inline void TextFileWriter::Close() {
    if (handle != nullptr) {
        std::fclose(AsTextFileHandle(handle));
        handle = nullptr;
    }
}

inline bool TextFileWriter::IsOpen() const {
    return handle != nullptr;
}

inline bool TextFileWriter::WriteLine(const std::string& line) {
    if (handle == nullptr) {
        return false;
    }

    std::FILE* file = AsTextFileHandle(handle);
    if (std::fputs(line.c_str(), file) < 0) {
        return false;
    }
    if (std::fputc('\n', file) == EOF) {
        return false;
    }
    return true;
}

inline bool TryReadWholeTextFile(const std::string& path, std::string& text) {
    TextFileReader reader;
    if (!reader.Open(path)) {
        return false;
    }

    text.clear();
    std::string line;
    bool firstLine = true;
    while (reader.ReadLine(line)) {
        if (!firstLine) {
            text += '\n';
        }
        text += line;
        firstLine = false;
    }

    return true;
}
