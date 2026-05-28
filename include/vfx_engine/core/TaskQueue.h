#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <optional>

namespace vfx {

class TaskQueue {
public:
    using Task = std::function<void()>;

    TaskQueue() = default;
    ~TaskQueue() {
        stop();
    }

    // Push a new task into the queue
    void push(Task task);

    // Pop a task from the queue, blocks if empty
    std::optional<Task> pop();

    // Stops the queue and wakes up waiting threads
    void stop();

private:
    std::queue<Task> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cond_var;
    bool m_stopped = false;
};

} // namespace vfx
