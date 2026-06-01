#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include "vfx_engine/media/VideoEncoder.h" // For VideoCodecType

namespace vfx {

class MediaEncoder {
public:
    virtual ~MediaEncoder() = default;

    // Initialize muxer, video codec, and optionally audio codec.
    virtual bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec = VideoCodecType::H264, bool enableAudio = true) = 0;

    // Stop recording, drain final frames, close muxer
    virtual void stop() = 0;

    // Periodically drain encoded packets from both video and audio encoders
    virtual void drain() = 0;

    // Feed PCM audio data to the audio encoder
    virtual void encodeAudioFrame(const int16_t* audioData, int numFrames, int64_t timestampNs) = 0;

    // Return the native window (e.g. ANativeWindow*) to be used as EGLSurface target for video
    virtual void* getInputWindow() = 0;

    // Notify that a video frame was drawn to the surface
    virtual void notifyFrameReady() = 0;

    static std::shared_ptr<MediaEncoder> create();
};

} // namespace vfx
