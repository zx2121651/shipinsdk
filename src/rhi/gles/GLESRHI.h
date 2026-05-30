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

    void swapBuffers() override;
    void swapEncoderBuffers() override;

    void makeMainWindowCurrent() override;
    void makeEncoderWindowCurrent() override;

    unsigned int compileShaderProgram(const char* vertexSource, const char* fragmentSource) override;
    void deleteShaderProgram(unsigned int programId) override;
    void drawFullScreenQuad(unsigned int programId, int textureId, bool isOES, const float* transformMatrix) override;

    std::shared_ptr<ITexture> createTexture(int width, int height) override;
    std::shared_ptr<ICommandBuffer> createCommandBuffer() override;
    std::shared_ptr<IPipelineState> createPipelineState() override;

    RHIBackend getBackendType() const override { return RHIBackend::GLES; }

private:
    bool setupVBO();

    void* m_eglDisplay;
    void* m_eglContext;
    void* m_eglConfig;

    void* m_eglSurfaceMain;
    void* m_eglSurfaceEncoder;

    unsigned int m_vbo = 0;
};

} // namespace vfx
