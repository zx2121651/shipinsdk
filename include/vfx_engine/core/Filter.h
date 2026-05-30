#pragma once

#include <memory>
#include <string>
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

// Represents context passed between filters during a render pass
struct RenderContext {
    std::shared_ptr<IRHI> rhi;
    int inputTextureId = -1;
    int outputTextureId = -1;
    const float* transformMatrix = nullptr;
    int width = 0;
    int height = 0;
};

class IFilter {
public:
    virtual ~IFilter() = default;

    // Called once during pipeline setup
    virtual bool initialize(std::shared_ptr<IRHI> rhi) = 0;

    // Called when the filter is removed or engine shuts down
    virtual void release() = 0;

    // Executes the filter logic for a single frame
    virtual void process(RenderContext& context) = 0;
};

} // namespace vfx
