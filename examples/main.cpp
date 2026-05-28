#include <chrono>
#include <iostream>
#include "vfx_engine/core/RenderThread.h"
#include "vfx_engine/rhi/RHI.h"
#include "vfx_engine/audio/AudioManager.h"
#include "vfx_engine/media/MediaManager.h"

int main() {
    std::cout << "VFX Engine Example App Initializing..." << std::endl;

    // Initialize subsystems
    auto rhi = vfx::createRHI(vfx::RHIBackend::Vulkan);
    rhi->initialize();

    vfx::AudioManager::getInstance().initialize();
    vfx::MediaManager::getInstance().initialize();

    // Start rendering thread (simulated)
    vfx::RenderThread renderThread;
    renderThread.start();

    renderThread.postTask([&rhi]() {
        auto cmd = rhi->createCommandBuffer();
        cmd->begin();
        std::cout << "[RenderThread] Recording graphic commands..." << std::endl;
        cmd->end();
        cmd->submit();
    });

    // Play media (simulated)
    auto player = vfx::MediaManager::getInstance().createMediaPlayer();
    player->load("dummy_video.mp4");
    player->play();

    // Sleep a bit to allow thread execution
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Shutdown
    renderThread.stop();
    rhi->shutdown();
    vfx::AudioManager::getInstance().shutdown();
    vfx::MediaManager::getInstance().shutdown();

    std::cout << "VFX Engine Example App Shutdown." << std::endl;
    return 0;
}
