#include "MetalRHI.h"

namespace vfx {

class MetalTexture : public ITexture {
public:
    void* getNativeHandle() const override { return nullptr; }
    TextureType getType() const override { return TextureType::Texture2D; }
    int getWidth() const override { return 0; }
    int getHeight() const override { return 0; }
};

class MetalRenderTarget : public IRenderTarget {
public:
    std::shared_ptr<ITexture> getTexture() const override { return nullptr; }
};

class MetalShader : public IShader {};

class MetalPipelineState : public IPipelineState {};

class MetalCommandBuffer : public ICommandBuffer {
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

bool MetalRHI::initialize(const HardwareCapabilities& caps) { return true; }
void MetalRHI::shutdown() {}
void MetalRHI::setWindow(void* window) {}
void MetalRHI::setEncoderWindow(void* window) {}
void MetalRHI::makeMainWindowCurrent() {}
void MetalRHI::makeEncoderWindowCurrent() {}
void MetalRHI::present(bool encoderSurface) {}

std::shared_ptr<IShader> MetalRHI::createShader(const std::string& vertexSource, const std::string& fragmentSource) {
    return std::make_shared<MetalShader>();
}

std::shared_ptr<IPipelineState> MetalRHI::createPipelineState(std::shared_ptr<IShader> shader) {
    return std::make_shared<MetalPipelineState>();
}

std::shared_ptr<ITexture> MetalRHI::createTexture(int width, int height, TextureType type) {
    return std::make_shared<MetalTexture>();
}

std::shared_ptr<ITexture> MetalRHI::createTextureFromNative(void* nativeHandle, int width, int height, TextureType type) {
    return std::make_shared<MetalTexture>();
}

std::shared_ptr<IRenderTarget> MetalRHI::createRenderTarget(int width, int height) {
    return std::make_shared<MetalRenderTarget>();
}

std::shared_ptr<ICommandBuffer> MetalRHI::createCommandBuffer() {
    return std::make_shared<MetalCommandBuffer>();
}

} // namespace vfx
