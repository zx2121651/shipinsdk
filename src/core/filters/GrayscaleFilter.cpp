#include "vfx_engine/core/filters/GrayscaleFilter.h"

namespace vfx {

static const char* VERTEX_SHADER = R"(
    attribute vec4 aPosition;
    attribute vec4 aTexCoord;
    varying vec2 vTexCoord;
    void main() {
        gl_Position = aPosition;
        vTexCoord = aTexCoord.xy;
    }
)";

static const char* GRAYSCALE_FRAGMENT_SHADER = R"(
    precision mediump float;
    varying vec2 vTexCoord;
    uniform sampler2D uTexture;
    void main() {
        vec4 color = texture2D(uTexture, vTexCoord);
        float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
        gl_FragColor = vec4(gray, gray, gray, color.a);
    }
)";

GrayscaleFilter::~GrayscaleFilter() {
    release();
}

bool GrayscaleFilter::initialize(std::shared_ptr<IRHI> rhi) {
    m_rhi = rhi;
    if (m_rhi) {
        m_programId = m_rhi->compileShaderProgram(VERTEX_SHADER, GRAYSCALE_FRAGMENT_SHADER);
        return m_programId != 0;
    }
    return false;
}

void GrayscaleFilter::release() {
    if (m_rhi && m_programId != 0) {
        m_rhi->deleteShaderProgram(m_programId);
        m_programId = 0;
    }
}

void GrayscaleFilter::process(RenderContext& context) {
    if (m_rhi && m_programId != 0 && context.inputTextureId >= 0) {
        // Draw applying grayscale filter.
        // isOES = false, because the input texture comes from the previous FBO, which is a standard GL_TEXTURE_2D.
        m_rhi->drawFullScreenQuad(m_programId, context.inputTextureId, false, nullptr);
    }
}

} // namespace vfx
