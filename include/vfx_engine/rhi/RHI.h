#pragma once

#include <memory>
#include <string>

namespace vfx {

enum class RHIBackend {
    Vulkan,
    Metal,
    GLES
};

class ITexture {
public:
    virtual ~ITexture() = default;
    virtual void* getNativeHandle() const = 0;
};

class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void submit() = 0;
};

class IPipelineState {
public:
    virtual ~IPipelineState() = default;
};

class IRHI {
public:
    virtual ~IRHI() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;

    virtual void setWindow(void* window) = 0;
    virtual void setEncoderWindow(void* window) = 0;

    virtual void swapBuffers() = 0;
    virtual void swapEncoderBuffers() = 0;

    virtual void renderCameraOESTexture(int textureId, const float* transformMatrix) = 0;

    virtual void makeMainWindowCurrent() = 0;
    virtual void makeEncoderWindowCurrent() = 0;

    virtual std::shared_ptr<ITexture> createTexture(int width, int height) = 0;
    virtual std::shared_ptr<ICommandBuffer> createCommandBuffer() = 0;
    virtual std::shared_ptr<IPipelineState> createPipelineState() = 0;

    virtual RHIBackend getBackendType() const = 0;
};

std::shared_ptr<IRHI> createRHI(RHIBackend backend);

} // namespace vfx
