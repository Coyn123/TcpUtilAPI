#pragma once
class TaskBase {
  public:
      virtual ~TaskBase() = default;
      virtual void run_task() = 0;
};
