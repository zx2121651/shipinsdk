#include "vfx_engine/audio/AudioManager.h"
#include <iostream>

namespace vfx {

class DummyAudioStream : public IAudioStream {
public:
    void play() override { std::cout << "AudioStream: play" << std::endl; }
    void pause() override { std::cout << "AudioStream: pause" << std::endl; }
    void stop() override { std::cout << "AudioStream: stop" << std::endl; }
};

bool AudioManager::initialize() {
    std::cout << "Initializing AudioManager (Oboe backend stub)..." << std::endl;
    return true;
}

void AudioManager::shutdown() {
    std::cout << "Shutting down AudioManager..." << std::endl;
}

std::shared_ptr<IAudioStream> AudioManager::createStream() {
    return std::make_shared<DummyAudioStream>();
}

} // namespace vfx
