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
        auto shader = m_rhi->createShader(VERTEX_SHADER, GRAYSCALE_FRAGMENT_SHADER);
        if (shader) {
            m_pso = m_rhi->createPipelineState(shader);
            return m_pso != nullptr;
        }
    }
    return false;
}

void GrayscaleFilter::release() {
    m_pso = nullptr;
    m_rhi = nullptr;
}

void GrayscaleFilter::process(RenderContext& context) {
    if (m_pso && context.inputTexture && context.cmdBuffer) {
        context.cmdBuffer->bindPipelineState(m_pso);
        context.cmdBuffer->bindTexture(0, context.inputTexture);
        context.cmdBuffer->drawFullScreenQuad();
    }
}

} // namespace vfx
