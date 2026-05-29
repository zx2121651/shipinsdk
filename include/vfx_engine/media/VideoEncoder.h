#pragma once

#include <string>
#include <memory>

namespace vfx {

enum class VideoCodecType {
    H264,
    H265
};

class VideoEncoder {
public:
    virtual ~VideoEncoder() = default;

    // Initialize codec & muxer. Attempts requestedCodec, falls back if unsupported.
    virtual bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec = VideoCodecType::H264) = 0;

    // Stop recording, drain final frames, close muxer
    virtual void stop() = 0;

    // Periodically drain encoded packets from the encoder and write to muxer
    virtual void drain() = 0;

    // Return the native window (e.g. ANativeWindow*) to be used as EGLSurface target
    virtual void* getInputWindow() = 0;

    // Set the presentation time for the current frame
    virtual void notifyFrameReady() = 0;

    static std::shared_ptr<VideoEncoder> create();
};

} // namespace vfx
