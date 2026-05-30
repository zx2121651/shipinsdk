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
        m_programId = m_rhi->compileShaderProgram(OES_VERTEX_SHADER, OES_FRAGMENT_SHADER);
        return m_programId != 0;
    }
    return false;
}

void OESCameraFilter::release() {
    if (m_rhi && m_programId != 0) {
        m_rhi->deleteShaderProgram(m_programId);
        m_programId = 0;
    }
}

void OESCameraFilter::process(RenderContext& context) {
    if (m_rhi && m_programId != 0 && context.inputTextureId >= 0) {
        // Draw the camera frame as a full screen quad
        m_rhi->drawFullScreenQuad(m_programId, context.inputTextureId, true, context.transformMatrix);
    }
}

} // namespace vfx
