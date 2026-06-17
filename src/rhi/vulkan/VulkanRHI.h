#pragma once
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

class VulkanRHI : public IRHI {
public:
    VulkanRHI() = default;
    ~VulkanRHI() override = default;

    bool initialize(const HardwareCapabilities& caps) override;
    void shutdown() override;

    void setWindow(void* window) override;
    void setEncoderWindow(void* window) override;

    void makeMainWindowCurrent() override;
    void makeEncoderWindowCurrent() override;

    void present(bool encoderSurface) override;

    std::shared_ptr<IShader> createShader(const std::string& vertexSource, const std::string& fragmentSource) override;
    std::shared_ptr<IPipelineState> createPipelineState(std::shared_ptr<IShader> shader) override;
    std::shared_ptr<ITexture> createTexture(int width, int height, TextureType type) override;
    std::shared_ptr<ITexture> createTextureFromNative(void* nativeHandle, int width, int height, TextureType type) override;
    std::shared_ptr<IRenderTarget> createRenderTarget(int width, int height) override;

    std::shared_ptr<ICommandBuffer> createCommandBuffer() override;

    RHIBackend getBackendType() const override { return RHIBackend::Vulkan; }
};

} // namespace vfx
