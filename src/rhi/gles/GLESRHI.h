#pragma once
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

class GLESRHI : public IRHI {
public:
    GLESRHI();
    ~GLESRHI() override;

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

    RHIBackend getBackendType() const override { return RHIBackend::GLES; }

    // Internal methods needed by GLESCommandBuffer
    void makeContextCurrent(bool encoderSurface);
    unsigned int getQuadVBO() const { return m_vbo; }

private:
    unsigned int m_vbo = 0;

    void* m_eglDisplay;
    void* m_eglContext;
    void* m_eglConfig;

    void* m_eglSurfaceMain;
    void* m_eglSurfaceEncoder;
};

} // namespace vfx
