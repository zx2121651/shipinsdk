#include <iostream>
#include <cassert>
#include <chrono>
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
    vfx::HardwareCapabilities caps;

    // Default or Auto might return GLES on non-Apple/non-Vulkan specific builds based on our current logic.
    // Let's test the explicit creation of GLES since that's our focus.
    auto glesRhi = vfx::createRHI(vfx::RHIBackend::GLES, caps);
    assert(glesRhi != nullptr);
    assert(glesRhi->getBackendType() == vfx::RHIBackend::GLES);
    std::cout << "GLES RHI creation successful." << std::endl;

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
