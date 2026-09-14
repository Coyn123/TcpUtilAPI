#pragma once
#include "core/Platform.h"
#include "Stream.h"

class Connection : public tcp::IStream {
    public:
        socket_t get() const noexcept;

        tcp::Result<size_t> read_some(char* buf, size_t len) override;
        tcp::Result<size_t> write_some(const char* buf, size_t len) override;

        //Explicit socket, no silent int conversion
        explicit Connection(socket_t fd);
        ~Connection() override;

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        Connection(Connection&&) noexcept;
        Connection& operator=(Connection&&) noexcept;

    private:
        socket_t fd_;
};
