#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <cstdint>

namespace vfx {

class AudioCapture {
public:
    // Callback definition: receives PCM 16-bit data, number of frames, and timestamp in nanoseconds
    using AudioDataCallback = std::function<void(const int16_t* audioData, int numFrames, int64_t timestampNs)>;

    virtual ~AudioCapture() = default;

    virtual bool start(int sampleRate, int channelCount) = 0;
    virtual void stop() = 0;
    virtual void setCallback(AudioDataCallback callback) = 0;

    static std::shared_ptr<AudioCapture> create();
};

} // namespace vfx
