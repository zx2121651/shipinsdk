#pragma once

#include <memory>

namespace vfx {

class IAudioStream {
public:
    virtual ~IAudioStream() = default;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
};

class AudioManager {
public:
    static AudioManager& getInstance() {
        static AudioManager instance;
        return instance;
    }

    // Initialize the audio backend (e.g., Oboe)
    bool initialize();

    // Shutdown the audio backend
    void shutdown();

    std::shared_ptr<IAudioStream> createStream();

private:
    AudioManager() = default;
    ~AudioManager() = default;
};

} // namespace vfx
