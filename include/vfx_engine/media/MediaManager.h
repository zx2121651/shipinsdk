#pragma once

#include <memory>
#include <string>

namespace vfx {

class IMediaPlayer {
public:
    virtual ~IMediaPlayer() = default;
    virtual bool load(const std::string& path) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    // For hardware decoding / async processing
    virtual void updateAsync() = 0;
};

class MediaManager {
public:
    static MediaManager& getInstance() {
        static MediaManager instance;
        return instance;
    }

    // Initialize the media backend (e.g., Media3, hardware decoders)
    bool initialize();

    // Shutdown the media backend
    void shutdown();

    std::shared_ptr<IMediaPlayer> createMediaPlayer();

private:
    MediaManager() = default;
    ~MediaManager() = default;
};

} // namespace vfx
