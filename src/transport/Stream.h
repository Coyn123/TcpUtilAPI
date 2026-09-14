#pragma once
#include "core/ResultType.h"
#include <cstddef>

namespace tcp {
    class IStream {
        public:
            virtual ~IStream() = default;

            virtual Result<size_t> read_some(char* buf, size_t len) = 0;
            virtual Result<size_t> write_some(const char* buf, size_t len) = 0;

            Result<void> write_all(const char* buf, size_t len);

    };
}
