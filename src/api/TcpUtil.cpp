#include "TcpUtil.h"
#include "transport/Listener.h"
#include "tasks/HttpTask.h"
#include <thread>
#include <cstdio>
#include <chrono>

TcpUtil::TcpUtil(uint16_t port, TaskFactory factory, size_t max_concurrent)
: port_(port), factory_(std::move(factory)), sem_(max_concurrent), max_concurrent_(max_concurrent) {}

tcp::Result<void> TcpUtil::run() {

    tcp::Result<Listener> made = Listener::create(port_);

    if (!made) {
        fprintf(stderr, "create failed: %s\n", strerror(made.error()));
        return tcp::Result<void>::err(made.error());
    }

    Listener listener = std::move(made.value());
    printf("listening on %d\n", port_);

    int listener_error = 0;

    for (;;) {
        printf("waiting for a client...\n");

        tcp::Result<Connection> incoming = listener.accept();
        if (!incoming) {
            if(incoming.error() == kFdExhausted) {
                fprintf(stderr, "File Descriptors exhausted, waiting: %s\n", strerror(incoming.error()));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            if(incoming.error() == kSysExhausted) {
                fprintf(stderr, "System-wide table full/exhausted, waiting: %s\n", strerror(incoming.error()));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            if(incoming.error() == kListenerBadFd || incoming.error() == kListenerNotSocket ||
               incoming.error() == kListenerOpNotSupported || incoming.error() == kListenerInvalid) {
                fprintf(stderr, "Listening socket is invalid, closing server: %s\n", strerror(incoming.error()));
                listener_error = incoming.error();
                break;
            }

            fprintf(stderr, "accept failed: %s\n", strerror(incoming.error()));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        Connection conn = std::move(incoming.value());
        printf("client connected\n");

        std::unique_ptr<TaskBase> task = factory_(std::move(conn));

        sem_.acquire();

        std::thread worker([t = std::move(task), this]() { t->run_task(); sem_.release(); });
        worker.detach();
    }
    //Drain
    for (size_t i = 0; i < max_concurrent_; i++) {
        sem_.acquire();
    }

    printf("\ndone -- listener closes as main returns\n");

    if (listener_error != 0) {
        return tcp::Result<void>::err(listener_error);
    }

    return tcp::Result<void>::ok();

}

std::unique_ptr<TaskBase> make_http_task(Connection conn) {
    return std::make_unique<HttpTask>(std::move(conn));
}
