#pragma once
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

class MetalRHI : public IRHI {
public:
    bool initialize() override;
    void shutdown() override;
    void setWindow(void* window) override;
    void setEncoderWindow(void* window) override;
    void swapBuffers() override;
    void swapEncoderBuffers() override;
    void renderCameraOESTexture(int textureId, const float* transformMatrix) override;
    void makeMainWindowCurrent() override {}
    void makeEncoderWindowCurrent() override {}

    std::shared_ptr<ITexture> createTexture(int width, int height) override;
    std::shared_ptr<ICommandBuffer> createCommandBuffer() override;
    std::shared_ptr<IPipelineState> createPipelineState() override;
    RHIBackend getBackendType() const override { return RHIBackend::Metal; }
};

} // namespace vfx
