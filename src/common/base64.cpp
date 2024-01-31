//
// Copyright © 2024 SHAO Liming <lmshao@163.com>. All rights reserved.
//

#include "base64.h"

static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string Base64::Encode(const uint8_t *input, size_t length)
{
    size_t size = ((length + 2) / 3) * 4;
    char *encoded = new char[size];

    size_t i, j;
    uint32_t octet[3];
    for (i = 0, j = 0; i < length; i += 3) {
        octet[0] = i < length ? input[i] : 0;
        octet[1] = (i + 1) < length ? input[i + 1] : 0;
        octet[2] = (i + 2) < length ? input[i + 2] : 0;

        uint32_t triple = (octet[0] << 16) + (octet[1] << 8) + octet[2];

        encoded[j++] = base64_chars[(triple >> 18) & 0x3F];
        encoded[j++] = base64_chars[(triple >> 12) & 0x3F];
        encoded[j++] = (i + 1) < length ? base64_chars[(triple >> 6) & 0x3F] : '=';
        encoded[j++] = (i + 2) < length ? base64_chars[triple & 0x3F] : '=';
    }

    std::string str(encoded, size);
    delete[] encoded;
    return str;
}

static int base64_decode_char(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    if (c == '=')
        return 0;
    return -1;
}

bool Base64::Decode(const std::string &input, std::vector<uint8_t> &output)
{
    if ((input.size() & 0x03) != 0) {
        return false;
    }

    size_t size = (input.size() / 4) * 3;
    if (input[input.size() - 1] == '=') {
        size--;
    }
    if (input[input.size() - 2] == '=') {
        size--;
    }

    uint8_t *decoded = new uint8_t[size];

    size_t i, j;
    int c0, c1, c2, c3, triple;
    for (i = 0, j = 0; i < input.size(); i += 4) {
        c0 = base64_decode_char(input[i]);
        c1 = base64_decode_char(input[i + 1]);
        c2 = base64_decode_char(input[i + 2]);
        c3 = base64_decode_char(input[i + 3]);

        if (c0 == -1 || c1 == -1 || c2 == -1 || c3 == -1) {
            delete[] decoded;
            return false;
        }

        triple = (c0 << 18) + (c1 << 12) + (c2 << 6) + c3;

        if (j < size) {
            decoded[j++] = (triple >> 16) & 0xFF;
        }
        if (j < size) {
            decoded[j++] = (triple >> 8) & 0xFF;
        }
        if (j < size) {
            decoded[j++] = triple & 0xFF;
        }
    }

    output.assign(decoded, decoded + size);
    delete[] decoded;
    return true;
}

bool Base64::Decode(const std::string &input, std::string &output)
{
    std::vector<uint8_t> encoded;
    bool res = Decode(input, encoded);
    if (res) {
        output.assign((char *)encoded.data(), encoded.size());
    }
    return res;
}