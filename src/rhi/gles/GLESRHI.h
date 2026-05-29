#pragma once
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

class GLESRHI : public IRHI {
public:
    GLESRHI();
    ~GLESRHI() override;

    bool initialize() override;
    void shutdown() override;

    void setWindow(void* window) override;
    void setEncoderWindow(void* window) override;

    void swapBuffers() override;
    void swapEncoderBuffers() override;

    void makeMainWindowCurrent() override;
    void makeEncoderWindowCurrent() override;

    void renderCameraOESTexture(int textureId, const float* transformMatrix) override;

    std::shared_ptr<ITexture> createTexture(int width, int height) override;
    std::shared_ptr<ICommandBuffer> createCommandBuffer() override;
    std::shared_ptr<IPipelineState> createPipelineState() override;

    RHIBackend getBackendType() const override { return RHIBackend::GLES; }

private:
    bool setupOESPipeline();

    void* m_eglDisplay;
    void* m_eglContext;
    void* m_eglConfig;

    void* m_eglSurfaceMain;
    void* m_eglSurfaceEncoder;

    unsigned int m_oesProgram = 0;
    unsigned int m_vbo = 0;
    int m_transformMatrixLocation = -1;
    int m_positionLocation = -1;
    int m_texCoordLocation = -1;
    int m_textureLocation = -1;
};

} // namespace vfx
