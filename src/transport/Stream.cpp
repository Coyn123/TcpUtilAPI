#include "Stream.h"

tcp::Result<void> tcp::IStream::write_all(const char* buf, size_t len) {
    size_t written = 0;
    do {

    tcp::Result<size_t> try_write = write_some(buf + written, len - written);
    if(!try_write) return tcp::Result<void>::err(try_write.error());
    written += try_write.value();

    } while (written < len);
    return tcp::Result<void>::ok();
}
