#pragma once

#include "TaskQueue.h"
#include <thread>
#include <atomic>
#include <functional>

namespace vfx {

class RenderThread {
public:
    RenderThread();
    ~RenderThread();

    // Start the render thread
    void start();

    // Stop the render thread
    void stop();

    // Post a task to be executed on the render thread
    void postTask(std::function<void()> task);

    // Join the thread
    void join();

private:
    void run();

    TaskQueue m_taskQueue;
    std::thread m_thread;
    std::atomic<bool> m_running;
};

} // namespace vfx
