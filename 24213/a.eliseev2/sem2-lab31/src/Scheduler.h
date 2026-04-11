#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <unordered_map>

namespace proxy {

class Task;
class Scheduler;

// A handle for waking up an associated task. Must not outlive the scheduler.
class Waker {
  public:
    Waker();
    Waker(std::shared_ptr<Task> task, Scheduler &scheduler);

    // Schedules the associated task for execution.
    void schedule();

  private:
    std::weak_ptr<Task> task;
    Scheduler *scheduler;
};

// An asynchronous task.
class Task {
  public:
    // Returns true if the task has completed.
    virtual bool service(Waker waker) = 0;
};

class Scheduler {
    friend class Waker;

  public:
    Scheduler();

    // Disable copy/move constructors/assignment operators.
    Scheduler(const Scheduler &) = delete;
    Scheduler(Scheduler &&) = delete;
    Scheduler &operator=(const Scheduler &) = delete;
    Scheduler &operator=(Scheduler &&) = delete;

    // Services a single task. Returns true if there are more tasks to service.
    bool service();

    // Starts a new task. The arguments are passed to the task's constructor.
    template <typename T, typename... Args> void startNew(Args &&...args) {
        std::shared_ptr<Task> task =
            std::static_pointer_cast<Task>(std::make_shared<T>(args...));
        tasks[task] = {
            .scheduled = true,
        };
        taskQueue.push_back(task);
    }

  private:
    struct TaskInfo {
        bool scheduled = false;
    };

    void schedule(std::shared_ptr<Task> task);

    std::unordered_map<std::shared_ptr<Task>, TaskInfo> tasks;
    std::deque<std::weak_ptr<Task>> taskQueue;
};

} // namespace proxy
