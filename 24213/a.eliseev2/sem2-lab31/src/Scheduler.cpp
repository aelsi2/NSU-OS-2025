#include "Scheduler.h"

namespace proxy {

Scheduler::Scheduler() : tasks(), taskQueue() {
}

bool Scheduler::service() {
    if (taskQueue.size() == 0) {
        return false;
    }
    auto it = tasks.find(taskQueue.front().lock());
    taskQueue.pop_front();
    if (it != tasks.end()) {
        std::shared_ptr<Task> task = it->first;
        it->second.scheduled = false;
        if (task->service(Waker(task, *this))) {
            tasks.erase(task);
        }
    }
    return taskQueue.size() != 0;
}

void Scheduler::schedule(std::shared_ptr<Task> task) {
    auto it = tasks.find(task);
    if (it == tasks.end() || it->second.scheduled) {
        return;
    }
    it->second.scheduled = true;
    taskQueue.push_back(task);
}

Waker::Waker() : task(), scheduler(nullptr) {
}

Waker::Waker(std::shared_ptr<Task> task, Scheduler &scheduler)
    : task(task), scheduler(&scheduler) {
}

void Waker::schedule() {
    std::shared_ptr<Task> maybeTask = task.lock();
    if (maybeTask && scheduler) {
        scheduler->schedule(maybeTask);
    }
}

} // namespace proxy
