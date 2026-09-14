#pragma once
#include <functional>
#include <memory>
#include <semaphore>
#include <cstdint>
#include "tasks/TaskBase.h"
#include "transport/Connection.h"
#include "core/ResultType.h"


std::unique_ptr<TaskBase> make_http_task(Connection conn);

class TcpUtil {
    public:
    typedef std::function<std::unique_ptr<TaskBase>(Connection)> TaskFactory;

    TcpUtil(uint16_t port, TaskFactory factory, size_t max_concurrent = 6);

    tcp::Result<void> run();

    private:
        uint16_t port_ = 0;
        std::function<std::unique_ptr<TaskBase>(Connection)> factory_;
        std::counting_semaphore<256> sem_;
        size_t max_concurrent_;

};
