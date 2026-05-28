#include "vfx_engine/core/TaskQueue.h"

namespace vfx {

void TaskQueue::push(Task task) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push(std::move(task));
    }
    m_cond_var.notify_one();
}

std::optional<TaskQueue::Task> TaskQueue::pop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond_var.wait(lock, [this]() {
        return m_stopped || !m_tasks.empty();
    });

    if (m_stopped && m_tasks.empty()) {
        return std::nullopt;
    }

    Task task = std::move(m_tasks.front());
    m_tasks.pop();
    return task;
}

void TaskQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = true;
    }
    m_cond_var.notify_all();
}

} // namespace vfx
