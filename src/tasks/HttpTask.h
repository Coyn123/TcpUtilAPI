#pragma once
#include "TaskBase.h"
#include "transport/Connection.h"

class HttpTask : public TaskBase {
    public:
        void run_task() override;
        explicit HttpTask(Connection conn);

    private:
        Connection connection_;
};
