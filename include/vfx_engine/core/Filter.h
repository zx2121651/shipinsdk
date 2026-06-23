#pragma once

#include <memory>
#include <string>
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

// Represents context passed between filters during a render pass
struct RenderContext {
    std::shared_ptr<IRHI> rhi;
    std::shared_ptr<ICommandBuffer> cmdBuffer; // The current command buffer being recorded into
    std::shared_ptr<ITexture> inputTexture;

    // Optional data like OES transform matrix from Android CameraX
    const float* transformMatrix = nullptr;

    int width = 0;
    int height = 0;

    bool isEncoderTarget = false; // Indicates if this render pass is destined for the video encoder
};

class IFilter {
public:
    virtual ~IFilter() = default;

    // Called once during pipeline setup
    virtual bool initialize(std::shared_ptr<IRHI> rhi) = 0;

    // Called when the filter is removed or engine shuts down
    virtual void release() = 0;

    // Records the filter's rendering commands into context.cmdBuffer
    virtual void process(RenderContext& context) = 0;
};

} // namespace vfx
