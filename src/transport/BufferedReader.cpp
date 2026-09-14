#include "BufferedReader.h"
#include "core/Platform.h"
#include "core/ResultType.h"
#include <algorithm>

BufferedReader::BufferedReader(tcp::IStream& stream) : stream_(stream) {}

std::string BufferedReader::split_on_delim(std::string& txt, size_t pos, const std::string& delim) {
    std::string ret = txt.substr(0, pos + delim.size());
    buffer_ = txt.substr(pos + delim.size());
    return ret;
}

tcp::Result<size_t> BufferedReader::read_one_more_chunk(char* chunk, size_t byte_max, size_t byte_check, size_t chunk_size) {
    tcp::Result<size_t> out = stream_.read_some( chunk, std::min(byte_max - byte_check, chunk_size) );
    if (!out) {
        return tcp::Result<size_t>::err(out.error());
    } else if (out.value() == 0) {
        return tcp::Result<size_t>::ok(0);
    }
    return out;
}

tcp::Result<tcp::BufferedResult> BufferedReader::read_exact(size_t byte_max) {
    char chunk[4096];
    std::string txt = buffer_;
    size_t byte_check = txt.size();
    buffer_.clear();

    while(byte_check < byte_max) {
        tcp::Result<size_t> out_n = read_one_more_chunk(chunk, byte_max, byte_check, sizeof(chunk));
        if (!out_n) {
            return tcp::Result<tcp::BufferedResult>::err(out_n.error());
        } else if(out_n.value() == 0) {
            return tcp::Result<tcp::BufferedResult>::ok({txt, false});
        }
        txt.append(chunk, out_n.value());
        byte_check += out_n.value();
    }
    return tcp::Result<tcp::BufferedResult>::ok({txt, true});
}

tcp::Result<tcp::BufferedResult> BufferedReader::read_until(const std::string& delim, size_t byte_max) {
    char chunk[4096];
    std::string txt = buffer_;
    size_t byte_check = txt.size();
    buffer_.clear();
    size_t pos = txt.find(delim);

    if (pos != std::string::npos) {
        std::string ret = split_on_delim(txt, pos, delim);
        return tcp::Result<tcp::BufferedResult>::ok({ret, true});
    }
    while(byte_check < byte_max && pos == std::string::npos) {
        tcp::Result<size_t> out_n = read_one_more_chunk(chunk, byte_max, byte_check, sizeof(chunk));
        if (!out_n) {
            return tcp::Result<tcp::BufferedResult>::err(out_n.error());
        } else if(out_n.value() == 0) {
            return tcp::Result<tcp::BufferedResult>::ok({txt, false});
        }
        txt.append(chunk, out_n.value());
        byte_check += out_n.value();
        pos = txt.find(delim);
        if (pos != std::string::npos) {
            std::string ret = split_on_delim(txt, pos, delim);
            return tcp::Result<tcp::BufferedResult>::ok({ret, true});
        }
    }
    return tcp::Result<tcp::BufferedResult>::ok({txt, false});

}
