//
// Copyright © 2023-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "file_io.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

std::shared_ptr<FileReader> FileReader::Open(const std::string &filename)
{
    int fd = open(filename.c_str(), O_RDWR);
    if (fd < 0) {
        perror("open");
        return nullptr;
    }

    struct stat sb {};
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        return nullptr;
    }

    void *memAddr = mmap(nullptr, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (memAddr == MAP_FAILED) {
        perror("mmap");
        return nullptr;
    }

    madvise(memAddr, sb.st_size, MADV_SEQUENTIAL);

    return std::shared_ptr<FileReader>(new FileReader((uint8_t *)memAddr, sb.st_size, fd));
}

void FileReader::Close()
{
    if (data) {
        munmap(data, size);
        close(fd_);
    }
    data = nullptr;
    size = 0;
    fd_ = 0;
}

FileReader::~FileReader()
{
    Close();
}

std::shared_ptr<FileWriter> FileWriter::Open(const std::string &filename)
{
    FILE *fd = fopen(filename.c_str(), "wb");
    if (!fd) {
        return nullptr;
    }

    return std::shared_ptr<FileWriter>(new FileWriter(fd));
}

bool FileWriter::Write(const uint8_t *data, size_t size)
{
    if (fd_) {
        size_t ret = fwrite(data, 1, size, fd_);
        if (ret != size) {
            totalSize_ += ret;
            if (totalSize_ >= 4096) {
                Flush();
            }
            return true;
        }
    }

    return false;
}

bool FileWriter::Write(const char *data, size_t size)
{
    return Write((const uint8_t *)data, size);
}

bool FileWriter::Write(const std::string &str)
{
    return Write(str.c_str(), str.size());
}

void FileWriter::Flush()
{
    if (fd_) {
        fflush(fd_);
    }
    totalSize_ = 0;
}

void FileWriter::Close()
{
    Flush();
    if (fd_) {
        fclose(fd_);
        fd_ = nullptr;
    }
}

FileWriter::~FileWriter()
{
    Close();
}