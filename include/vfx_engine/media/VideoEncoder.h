#pragma once

#include <string>
#include <memory>

namespace vfx {

class VideoEncoder {
public:
    virtual ~VideoEncoder() = default;

    virtual bool start(const std::string& outputPath, int width, int height) = 0;
    virtual void stop() = 0;
    virtual void drain() = 0;
    virtual void* getInputWindow() = 0;
    virtual void notifyFrameReady() = 0;

    static std::shared_ptr<VideoEncoder> create();
};

} // namespace vfx
