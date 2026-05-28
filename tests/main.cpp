#include <chrono>
#include <iostream>
#include <cassert>
#include "vfx_engine/core/RenderThread.h"
#include "vfx_engine/rhi/RHI.h"

int main() {
    std::cout << "Starting VFX Engine Tests..." << std::endl;

    // Test Threading
    vfx::RenderThread renderThread;
    renderThread.start();

    std::atomic<bool> taskExecuted{false};
    renderThread.postTask([&taskExecuted]() {
        std::cout << "Task executed on RenderThread." << std::endl;
        taskExecuted = true;
    });

    // Wait briefly for task
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(taskExecuted == true);

    renderThread.stop();

    // Test RHI Factory
    auto rhi = vfx::createRHI(vfx::RHIBackend::Vulkan);
    assert(rhi != nullptr);
    assert(rhi->getBackendType() == vfx::RHIBackend::Vulkan);

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
