#pragma once
#include "Connection.h"
#include <cstdint>
#include "core/ResultType.h"

class Listener {
    public:
        static tcp::Result<Listener> create(uint16_t port);
        tcp::Result<Connection> accept();

        ~Listener();
        Listener(const Listener&) = delete;
        Listener& operator=(const Listener&) = delete;
        Listener(Listener&&) noexcept;
        Listener& operator=(Listener&&) noexcept;

        socket_t get() const noexcept;

    private:
        explicit Listener(socket_t fd);
        socket_t fd_;
};
