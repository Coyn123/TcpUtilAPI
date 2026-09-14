#pragma once
#include "Stream.h"
#include <string>
namespace tcp {
    struct BufferedResult {
        std::string bytes;
        bool complete;
    };
}

class BufferedReader {
    public:
        explicit BufferedReader(tcp::IStream& stream);

        BufferedReader(const BufferedReader&) = delete;
        BufferedReader& operator=(const BufferedReader&) = delete;

        BufferedReader(BufferedReader&&) noexcept = delete;
        BufferedReader& operator=(BufferedReader&&) noexcept = delete;

        tcp::Result<tcp::BufferedResult> read_until(const std::string& delim, size_t byte_max);
        tcp::Result<tcp::BufferedResult> read_exact(size_t byte_max);

    private:
        std::string split_on_delim(std::string& txt, size_t pos, const std::string& delim);
        tcp::Result<size_t> read_one_more_chunk(char* chunk, size_t byte_max, size_t byte_check, size_t chunk_size);

        tcp::IStream& stream_;
        std::string buffer_;

};
