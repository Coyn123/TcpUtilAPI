#include "Listener.h"
#include "core/Platform.h"

tcp::Result<Listener> Listener::create(uint16_t port) {

    tcp::ensure_started();

    socket_t fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    //Always close fd as no deconstructor owns the cleanup here, we are creating a static factory for objects,
    //We still own the fd value. Benefit of the factory: errors out to no object, instead of a partial object with incomplete values
    //After the object is created the deconstructor owns the fd cleanup

    //Check socket validity
    if (fd == kInvalidSocket) {
        int e = last_error();
        return tcp::Result<Listener>::err(e);
    }

    //Socket configuartion validity
    if (set_reuseaddr(fd) != 0) {
        int e = last_error();
        close_socket(fd);
        return tcp::Result<Listener>::err(e);
    }

    //Socket address validity
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if(bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        int e = last_error();
        close_socket(fd);
        return tcp::Result<Listener>::err(e);
    }

    //Listener + max connection validity
    if(listen(fd, SOMAXCONN) != 0) {
        int e = last_error();
        close_socket(fd);
        return tcp::Result<Listener>::err(e);
    }

    return tcp::Result<Listener>::ok(Listener(fd));
}

tcp::Result<Connection> Listener::accept() {
    socket_t fd = ::accept(fd_, nullptr, nullptr);
    if (fd == kInvalidSocket) {
        int e = last_error();
        return tcp::Result<Connection>::err(e);
    }
    return tcp::Result<Connection>::ok(Connection(fd));
}

Listener::~Listener() {
    if (fd_ != kInvalidSocket) {
        close_socket(fd_);
    }
}

Listener::Listener(Listener&& other) noexcept: fd_(other.fd_) {
    other.fd_ = kInvalidSocket;
}

Listener::Listener(socket_t fd): fd_(fd) {}

Listener& Listener::operator=(Listener&& other) noexcept {

    socket_t tmp = other.fd_;
    other.fd_ = kInvalidSocket;
    if(fd_ != kInvalidSocket) {
        close_socket(fd_);
    }
    fd_ = tmp;
    return *this;
}

socket_t Listener::get() const noexcept {
    return fd_;
}
