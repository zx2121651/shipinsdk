#pragma once
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

class MetalRHI : public IRHI {
public:
    bool initialize(const HardwareCapabilities& caps) override;
    void shutdown() override;
    void setWindow(void* window) override;
    void setEncoderWindow(void* window) override;
    void swapBuffers() override;
    void swapEncoderBuffers() override;
    void makeMainWindowCurrent() override {}
    void makeEncoderWindowCurrent() override {}

    unsigned int compileShaderProgram(const char* vertexSource, const char* fragmentSource) override { return 0; }
    void deleteShaderProgram(unsigned int programId) override {}
    void drawFullScreenQuad(unsigned int programId, int textureId, bool isOES, const float* transformMatrix) override {}

    std::shared_ptr<IRenderTarget> createRenderTarget(int width, int height) override { return nullptr; }
    void bindRenderTarget(std::shared_ptr<IRenderTarget> target) override {}
    void unbindRenderTarget() override {}

    std::shared_ptr<ITexture> createTexture(int width, int height) override;
    std::shared_ptr<ICommandBuffer> createCommandBuffer() override;
    std::shared_ptr<IPipelineState> createPipelineState() override;
    RHIBackend getBackendType() const override { return RHIBackend::Metal; }
};

} // namespace vfx
