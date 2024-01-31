//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#ifndef HALFWAY_MEDIA_BASE64_H
#define HALFWAY_MEDIA_BASE64_H

#include <cstdint>
#include <string>
#include <vector>

class Base64 {
public:
    static std::string Encode(const uint8_t *input, size_t length);
    static bool Decode(const std::string &input, std::string &output);
    static bool Decode(const std::string &input, std::vector<uint8_t> &output);
};

#endif // HALFWAY_MEDIA_BASE64_H