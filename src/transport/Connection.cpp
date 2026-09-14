#include "Connection.h"
#include "core/Platform.h"
#include <string>

Connection::Connection(socket_t fd): fd_(fd) {
    set_macopt(fd_);
}

Connection::~Connection() {
    if(fd_ != kInvalidSocket) {
        close_socket(fd_);
    }
}

Connection::Connection(Connection&& other) noexcept: fd_(other.fd_) {
    other.fd_ = kInvalidSocket;
}

Connection& Connection::operator=(Connection&& other) noexcept {

    socket_t tmp = other.fd_;
    other.fd_ = kInvalidSocket;
    if(fd_ != kInvalidSocket) {
        close_socket(fd_);
    }
    fd_ = tmp;
    return *this;
}

tcp::Result<size_t> Connection::read_some(char* buf, size_t len) {
    for (;;) {
        auto resp = recv(fd_, buf, len, 0);
        int e = last_error();
        if (resp < 0 and e == EINTR) {
            continue;
        }
        if(resp < 0 && e != EINTR) {
            return tcp::Result<size_t>::err(e);
            //EOF flow --> ok(0)
        } else if(resp == 0) return tcp::Result<size_t>::ok(0);
        else return tcp::Result<size_t>::ok(resp);
    };
}
tcp::Result<size_t> Connection::write_some(const char* buf, size_t len) {
    for(;;) {
        auto tryWrite = send(fd_, buf, len, kNoSignalFlag);
        if (tryWrite >= 0) {
            return tcp::Result<size_t>::ok(tryWrite);
        } else {
            int e = last_error();
            if (e == EINTR) { continue; }
            else return tcp::Result<size_t>::err(e);
        }
    }
}
//Getter
socket_t Connection::get() const noexcept {
    return fd_;
}
