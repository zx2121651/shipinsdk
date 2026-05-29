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

    // Set the native window (e.g., ANativeWindow* on Android, HWND on Windows)
    virtual void setWindow(void* window) = 0;

    // Present the backbuffer to the screen
    virtual void swapBuffers() = 0;

    // Render an external camera texture (OES on Android)
    // transformMatrix is a 4x4 column-major float array
    virtual void renderCameraOESTexture(int textureId, const float* transformMatrix) = 0;

    virtual std::shared_ptr<ITexture> createTexture(int width, int height) = 0;
    virtual std::shared_ptr<ICommandBuffer> createCommandBuffer() = 0;
    virtual std::shared_ptr<IPipelineState> createPipelineState() = 0;

    virtual RHIBackend getBackendType() const = 0;
};

// Factory function
std::shared_ptr<IRHI> createRHI(RHIBackend backend);

} // namespace vfx
