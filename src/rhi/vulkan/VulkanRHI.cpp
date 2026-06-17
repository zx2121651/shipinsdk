#include "VulkanRHI.h"

namespace vfx {

class VulkanTexture : public ITexture {
public:
    void* getNativeHandle() const override { return nullptr; }
    TextureType getType() const override { return TextureType::Texture2D; }
    int getWidth() const override { return 0; }
    int getHeight() const override { return 0; }
};

class VulkanRenderTarget : public IRenderTarget {
public:
    std::shared_ptr<ITexture> getTexture() const override { return nullptr; }
};

class VulkanShader : public IShader {};

class VulkanPipelineState : public IPipelineState {};

class VulkanCommandBuffer : public ICommandBuffer {
public:
    void begin() override {}
    void beginRenderPass(const RenderPassDescriptor& desc) override {}
    void bindPipelineState(std::shared_ptr<IPipelineState> pso) override {}
    void bindTexture(int slot, std::shared_ptr<ITexture> texture) override {}
    void pushConstants(const void* data, size_t size) override {}
    void drawFullScreenQuad() override {}
    void endRenderPass() override {}
    void end() override {}
    void submit() override {}
};

bool VulkanRHI::initialize(const HardwareCapabilities& caps) { return true; }
void VulkanRHI::shutdown() {}
void VulkanRHI::setWindow(void* window) {}
void VulkanRHI::setEncoderWindow(void* window) {}
void VulkanRHI::makeMainWindowCurrent() {}
void VulkanRHI::makeEncoderWindowCurrent() {}
void VulkanRHI::present(bool encoderSurface) {}

std::shared_ptr<IShader> VulkanRHI::createShader(const std::string& vertexSource, const std::string& fragmentSource) {
    return std::make_shared<VulkanShader>();
}

std::shared_ptr<IPipelineState> VulkanRHI::createPipelineState(std::shared_ptr<IShader> shader) {
    return std::make_shared<VulkanPipelineState>();
}

std::shared_ptr<ITexture> VulkanRHI::createTexture(int width, int height, TextureType type) {
    return std::make_shared<VulkanTexture>();
}

std::shared_ptr<ITexture> VulkanRHI::createTextureFromNative(void* nativeHandle, int width, int height, TextureType type) {
    return std::make_shared<VulkanTexture>();
}

std::shared_ptr<IRenderTarget> VulkanRHI::createRenderTarget(int width, int height) {
    return std::make_shared<VulkanRenderTarget>();
}

std::shared_ptr<ICommandBuffer> VulkanRHI::createCommandBuffer() {
    return std::make_shared<VulkanCommandBuffer>();
}

} // namespace vfx
