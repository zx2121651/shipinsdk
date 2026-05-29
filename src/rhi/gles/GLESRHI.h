#pragma once
#include "vfx_engine/rhi/RHI.h"

// Note: To remain cross-platform (e.g. non-Android), one would typically use
// EGL or WGL conditionally. We assume EGL for standard GLES implementation.
// We use void* to store handles avoiding direct #include <EGL/egl.h> in headers.

namespace vfx {

class GLESRHI : public IRHI {
public:
    GLESRHI();
    ~GLESRHI() override;

    bool initialize() override;
    void shutdown() override;
    void setWindow(void* window) override;
    void swapBuffers() override;

    std::shared_ptr<ITexture> createTexture(int width, int height) override;
    std::shared_ptr<ICommandBuffer> createCommandBuffer() override;
    std::shared_ptr<IPipelineState> createPipelineState() override;

    RHIBackend getBackendType() const override { return RHIBackend::GLES; }

private:
    void* m_eglDisplay;
    void* m_eglContext;
    void* m_eglSurface;
    void* m_eglConfig;
};

} // namespace vfx
