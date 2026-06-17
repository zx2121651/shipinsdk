#include "vfx_engine/core/filters/OESCameraFilter.h"
#include <iostream>

namespace vfx {

static const char* OES_VERTEX_SHADER = R"(
    attribute vec4 aPosition;
    attribute vec4 aTexCoord;
    uniform mat4 uTransformMatrix;
    varying vec2 vTexCoord;
    void main() {
        gl_Position = aPosition;
        vTexCoord = (uTransformMatrix * aTexCoord).xy;
    }
)";

static const char* OES_FRAGMENT_SHADER = R"(
    #extension GL_OES_EGL_image_external : require
    precision mediump float;
    varying vec2 vTexCoord;
    uniform samplerExternalOES uTexture;
    void main() {
        gl_FragColor = texture2D(uTexture, vTexCoord);
    }
)";

OESCameraFilter::~OESCameraFilter() {
    release();
}

bool OESCameraFilter::initialize(std::shared_ptr<IRHI> rhi) {
    m_rhi = rhi;
    if (m_rhi) {
        auto shader = m_rhi->createShader(OES_VERTEX_SHADER, OES_FRAGMENT_SHADER);
        if (shader) {
            m_pso = m_rhi->createPipelineState(shader);
            return m_pso != nullptr;
        }
    }
    return false;
}

void OESCameraFilter::release() {
    m_pso = nullptr;
    m_rhi = nullptr;
}

void OESCameraFilter::process(RenderContext& context) {
    if (m_pso && context.inputTexture && context.cmdBuffer) {
        context.cmdBuffer->bindPipelineState(m_pso);

        if (context.transformMatrix) {
            context.cmdBuffer->pushConstants(context.transformMatrix, 16 * sizeof(float));
        }

        context.cmdBuffer->bindTexture(0, context.inputTexture);
        context.cmdBuffer->drawFullScreenQuad();
    }
}

} // namespace vfx
