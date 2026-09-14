#include "Base64.h"
#include <cstdint>


namespace {
static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}


std::string base64_encode(const unsigned char* data, size_t len) {

    //Precompute out size from len in
    std::string ret;
    ret.reserve(((len + 2) / 3) * 4);
    size_t full_groups = len / 3;
    size_t leftover = len % 3;

    for(size_t i = 0; i < full_groups; i++) {

        uint32_t b0 = data[i*3];
        uint32_t b1 = data[i*3+1];
        uint32_t b2 = data[i*3+2];
        uint32_t out = (b0 << 16) | (b1 << 8) | b2;



        uint32_t chunk1 = (out >> 18) & 0x3F;
        ret += alphabet[chunk1];
        uint32_t chunk2 = (out >> 12) & 0x3F;
        ret += alphabet[chunk2];
        uint32_t chunk3 = (out >> 6) & 0x3F;
        ret += alphabet[chunk3];
        uint32_t chunk4 = (out >> 0) & 0x3F;
        ret += alphabet[chunk4];

    }

    if(leftover > 0) {

        if (leftover == 1) {
            uint32_t b0 = data[full_groups*3];
            uint32_t b1 = 0;
            uint32_t b2 = 0;

            uint32_t out = (b0 << 16) | (b1 << 8) | b2;

            uint32_t chunk1 = (out >> 18) & 0x3F;
            ret += alphabet[chunk1];
            uint32_t chunk2 = (out >> 12) & 0x3F;
            ret += alphabet[chunk2];

            ret += "==";
        }

        if(leftover == 2) {
            uint32_t b0 = data[full_groups*3];
            uint32_t b1 = data[full_groups*3+1];
            uint32_t b2 = 0;

            uint32_t out = (b0 << 16) | (b1 << 8) | b2;

            uint32_t chunk1 = (out >> 18) & 0x3F;
            ret += alphabet[chunk1];
            uint32_t chunk2 = (out >> 12) & 0x3F;
            ret += alphabet[chunk2];
            uint32_t chunk3 = (out >> 6) & 0x3F;
            ret += alphabet[chunk3];

            ret += "=";
        }

    }
    return ret;
}
