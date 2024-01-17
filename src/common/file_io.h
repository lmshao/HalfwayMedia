//
// Copyright © 2023-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_FILE_IO_H
#define HALFWAY_MEDIA_FILE_IO_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class FileReader {
public:
    static std::shared_ptr<FileReader> Open(const std::string &filename);
    void Close();

    ~FileReader();

private:
    FileReader(uint8_t *data, size_t size, int fd) : data(data), size(size), fd_(fd) {}
    FileReader() = default;

public:
    uint8_t *data = nullptr;
    size_t size = 0;

private:
    int fd_ = 0;
};

class FileWriter {
public:
    static std::shared_ptr<FileWriter> Open(const std::string &filename);

    bool Write(const uint8_t *data, size_t size);
    bool Write(const char *data, size_t size);
    bool Write(const std::string &str);
    void Flush();
    void Close();

    ~FileWriter();

private:
    explicit FileWriter(FILE *fd) : fd_(fd) {}

private:
    FILE *fd_ = nullptr;
    size_t totalSize_ = 0;
};

#endif // HALFWAY_MEDIA_FILE_IO_H