#include "vfx_engine/media/MediaManager.h"
#include <iostream>

namespace vfx {

class DummyMediaPlayer : public IMediaPlayer {
public:
    bool load(const std::string& path) override {
        std::cout << "MediaPlayer: loading " << path << std::endl;
        return true;
    }
    void play() override { std::cout << "MediaPlayer: play" << std::endl; }
    void pause() override { std::cout << "MediaPlayer: pause" << std::endl; }
    void stop() override { std::cout << "MediaPlayer: stop" << std::endl; }
    void updateAsync() override { /* Process in background task queue */ }
};

bool MediaManager::initialize() {
    std::cout << "Initializing MediaManager (Media3 backend stub)..." << std::endl;
    return true;
}

void MediaManager::shutdown() {
    std::cout << "Shutting down MediaManager..." << std::endl;
}

std::shared_ptr<IMediaPlayer> MediaManager::createMediaPlayer() {
    return std::make_shared<DummyMediaPlayer>();
}

} // namespace vfx
