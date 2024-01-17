//
// Copyright © 2023-2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_DATA_BUFFER_H
#define HALFWAY_MEDIA_DATA_BUFFER_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

class DataBuffer {
public:
    explicit DataBuffer(size_t len = 0);
    DataBuffer(const DataBuffer &other) noexcept;
    DataBuffer &operator=(const DataBuffer &other) noexcept;
    DataBuffer(DataBuffer &&other) noexcept;
    DataBuffer &operator=(DataBuffer &&other) noexcept;

    virtual ~DataBuffer();

    static std::shared_ptr<DataBuffer> Create(size_t len = 0);

    void Assign(const void *p, size_t len);
    void Assign(uint8_t c) { Assign((uint8_t *)&c, 1); }
    void Assign(const char *str) { Assign(str, strlen(str)); }
    void Assign(const std::string &s) { Assign(s.c_str(), s.size()); }

    void Append(const void *p, size_t len);
    void Append(uint8_t c) { Append((uint8_t *)&c, 1); }
    void Append(const char *str) { Append(str, strlen(str)); }
    void Append(const std::string &s) { Append(s.c_str(), s.size()); }

    uint8_t *Data() { return data_; }

    size_t Size() const { return size_; }
    void SetSize(size_t len);

    size_t Capacity() const { return capacity_; }
    void SetCapacity(size_t len);

    void Clear();
    void HexDump(size_t len = 0);

private:
    uint8_t *data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

#endif // HALFWAY_MEDIA_DATA_BUFFER_H