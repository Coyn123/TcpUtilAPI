#include "Sha1.h"
#include <cstdint>
#include <vector>

namespace {

uint32_t rotate_left(uint32_t value, int bits) {
    return (value << bits) | (value >> (32 - bits));
}

}

std::array<unsigned char, 20> sha1(const unsigned char* data, size_t len) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    // padding 0x80 then zeros until len % 64 == 56 then 8-byte big-endian bit length
    uint64_t bit_len = static_cast<uint64_t>(len) * 8;

    std::vector<unsigned char> msg(data, data + len);
    msg.push_back(0x80);
    while (msg.size() % 64 != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<unsigned char>((bit_len >> (i * 8)) & 0xFF));
    }

    // process each 512-bit block
    for (size_t block = 0; block < msg.size(); block += 64) {
        uint32_t w[80];

        for (int i = 0; i < 16; ++i) {
            size_t offset = block + i * 4;
            w[i] = (static_cast<uint32_t>(msg[offset])     << 24) |
                   (static_cast<uint32_t>(msg[offset + 1]) << 16) |
                   (static_cast<uint32_t>(msg[offset + 2]) << 8)  |
                   (static_cast<uint32_t>(msg[offset + 3]));
        }

        for (int i = 16; i < 80; ++i) {
            w[i] = rotate_left(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = rotate_left(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotate_left(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    // big-endian
    std::array<unsigned char, 20> digest;
    uint32_t hs[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        digest[i * 4 + 0] = static_cast<unsigned char>((hs[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<unsigned char>((hs[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<unsigned char>((hs[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<unsigned char>(hs[i] & 0xFF);
    }
    return digest;
}
