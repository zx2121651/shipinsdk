#pragma once

#include <memory>
#include <string>

namespace vfx {

enum class RHIBackend {
    Auto,
    Vulkan,
    Metal,
    GLES
};

struct HardwareCapabilities {
    bool isVulkanSupported = false;
    int glesVersionHex = 0x00020000;
};

enum class TextureType {
    Texture2D,
    TextureExternal // e.g. OES on Android, CVPixelBuffer on iOS
};

class ITexture {
public:
    virtual ~ITexture() = default;
    virtual void* getNativeHandle() const = 0;
    virtual TextureType getType() const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
};

class IShader {
public:
    virtual ~IShader() = default;
};

class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;
    virtual std::shared_ptr<ITexture> getTexture() const = 0;
};

class IPipelineState {
public:
    virtual ~IPipelineState() = default;
};

// Represents a render pass (e.g. rendering to a texture or the main screen)
struct RenderPassDescriptor {
    std::shared_ptr<IRenderTarget> colorAttachment = nullptr; // nullptr means default backbuffer
    bool clearColor = false;
    float clearColorValue[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    bool isEncoderTarget = false; // Internal flag to identify if we are rendering to the secondary EGL surface
};

class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    // Begin recording commands
    virtual void begin() = 0;

    // Bind render targets and clear
    virtual void beginRenderPass(const RenderPassDescriptor& desc) = 0;

    virtual void bindPipelineState(std::shared_ptr<IPipelineState> pso) = 0;

    // Bind a texture to a specific binding slot (e.g. uniform location / descriptor set)
    virtual void bindTexture(int slot, std::shared_ptr<ITexture> texture) = 0;

    // For legacy/quick matrix passing (like OES transform)
    virtual void pushConstants(const void* data, size_t size) = 0;

    // Issue draw call (e.g. drawing a full screen quad)
    // We abstract vertex buffers away for now assuming a default full-screen quad is always bound for video filters.
    virtual void drawFullScreenQuad() = 0;

    virtual void endRenderPass() = 0;

    // Finish recording
    virtual void end() = 0;

    // Submit to the GPU queue
    virtual void submit() = 0;
};

class IRHI {
public:
    virtual ~IRHI() = default;

    virtual bool initialize(const HardwareCapabilities& caps) = 0;
    virtual void shutdown() = 0;

    virtual void setWindow(void* window) = 0;
    virtual void setEncoderWindow(void* window) = 0;

    virtual void makeMainWindowCurrent() = 0;
    virtual void makeEncoderWindowCurrent() = 0;

    // Swap presentation buffers for the specified target type
    virtual void present(bool encoderSurface) = 0;

    // Creation API
    virtual std::shared_ptr<IShader> createShader(const std::string& vertexSource, const std::string& fragmentSource) = 0;
    virtual std::shared_ptr<IPipelineState> createPipelineState(std::shared_ptr<IShader> shader) = 0;
    virtual std::shared_ptr<ITexture> createTexture(int width, int height, TextureType type = TextureType::Texture2D) = 0;
    // Create an ITexture wrapper around an externally generated texture ID (like an OES texture)
    virtual std::shared_ptr<ITexture> createTextureFromNative(void* nativeHandle, int width, int height, TextureType type) = 0;
    virtual std::shared_ptr<IRenderTarget> createRenderTarget(int width, int height) = 0;

    virtual std::shared_ptr<ICommandBuffer> createCommandBuffer() = 0;

    virtual RHIBackend getBackendType() const = 0;
};

std::shared_ptr<IRHI> createRHI(RHIBackend backend, const HardwareCapabilities& caps);

} // namespace vfx
