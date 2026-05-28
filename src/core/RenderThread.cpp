#include "vfx_engine/core/RenderThread.h"

namespace vfx {

RenderThread::RenderThread() : m_running(false) {
}

RenderThread::~RenderThread() {
    stop();
    join();
}

void RenderThread::start() {
    if (!m_running.exchange(true)) {
        m_thread = std::thread(&RenderThread::run, this);
    }
}

void RenderThread::stop() {
    if (m_running.exchange(false)) {
        m_taskQueue.stop();
    }
}

void RenderThread::postTask(std::function<void()> task) {
    if (m_running) {
        m_taskQueue.push(std::move(task));
    }
}

void RenderThread::join() {
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void RenderThread::run() {
    while (m_running) {
        auto task = m_taskQueue.pop();
        if (task) {
            (*task)();
        } else {
            break;
        }
    }
}

} // namespace vfx
