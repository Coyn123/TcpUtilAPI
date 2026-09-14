#pragma once
#include "TaskBase.h"
#include "transport/Connection.h"

class WSTask : public TaskBase {
    public:
        void run_task() override;
        explicit WSTask(Connection conn);

    private:
        Connection connection_;
};
