#include "vfx_engine/core/filters/BeautyFilter.h"
#include <vector>

namespace vfx {

static const char* BEAUTY_VERTEX_SHADER = R"(
    attribute vec4 aPosition;
    attribute vec4 aTexCoord;
    varying vec2 vTexCoord;

    // For single-pass blur offsets
    uniform vec2 uTexelSize;
    varying vec2 vBlurCoord[4];

    void main() {
        gl_Position = aPosition;
        vTexCoord = aTexCoord.xy;

        // Precompute texture coordinate offsets for a simple 5-tap cross blur
        vBlurCoord[0] = vTexCoord + vec2(uTexelSize.x * 2.0, 0.0);
        vBlurCoord[1] = vTexCoord - vec2(uTexelSize.x * 2.0, 0.0);
        vBlurCoord[2] = vTexCoord + vec2(0.0, uTexelSize.y * 2.0);
        vBlurCoord[3] = vTexCoord - vec2(0.0, uTexelSize.y * 2.0);
    }
)";

// A highly simplified single-pass edge-preserving blur (High Pass Skin Smoothing) + Lut/Curve based whitening approximation
static const char* BEAUTY_FRAGMENT_SHADER = R"(
    precision highp float;
    varying vec2 vTexCoord;
    varying vec2 vBlurCoord[4];

    uniform sampler2D uTexture;
    uniform float uSmoothing;
    uniform float uWhitening;

    void main() {
        vec4 centralColor = texture2D(uTexture, vTexCoord);

        // --- 1. Basic Smoothing (磨皮 - approximated bilateral/high-pass filter) ---
        vec3 sumColor = centralColor.rgb;
        float totalWeight = 1.0;

        for (int i = 0; i < 4; i++) {
            vec3 sampleColor = texture2D(uTexture, vBlurCoord[i]).rgb;
            // Edge preservation: weight drops if color difference is large (edges)
            float diff = length(centralColor.rgb - sampleColor);
            float weight = exp(-diff * diff * 50.0); // 50.0 is an arbitrary spatial variance
            sumColor += sampleColor * weight;
            totalWeight += weight;
        }

        vec3 smoothedColor = sumColor / totalWeight;

        // Blend original with smoothed based on smoothing parameter
        vec3 finalColor = mix(centralColor.rgb, smoothedColor, uSmoothing);

        // --- 2. Basic Whitening (美白 - Screen blend + curve adjustment) ---
        // Simple screen blend formula: 1 - (1 - a)(1 - b)
        vec3 whiteColor = finalColor + finalColor - finalColor * finalColor;

        // Optional: reduce reds/yellows slightly if it looks too warm (Log Curve mapping approx)
        whiteColor = mix(finalColor, whiteColor, uWhitening * 0.5);

        gl_FragColor = vec4(whiteColor, centralColor.a);
    }
)";

BeautyFilter::~BeautyFilter() {
    release();
}

bool BeautyFilter::initialize(std::shared_ptr<IRHI> rhi) {
    m_rhi = rhi;
    if (m_rhi) {
        auto shader = m_rhi->createShader(BEAUTY_VERTEX_SHADER, BEAUTY_FRAGMENT_SHADER);
        if (shader) {
            m_pso = m_rhi->createPipelineState(shader);
            return m_pso != nullptr;
        }
    }
    return false;
}

void BeautyFilter::release() {
    m_pso = nullptr;
    m_rhi = nullptr;
}

void BeautyFilter::process(RenderContext& context) {
    if (m_pso && context.inputTexture && context.cmdBuffer) {
        context.cmdBuffer->bindPipelineState(m_pso);

        // In a real pure RHI, pushConstants/Uniforms would be sent via structured buffers.
        // For our GLES-backed RHI, we will temporarily rely on pushConstants array layout to pass these 4 floats.
        // Struct: [ texelWidth, texelHeight, smoothing, whitening ]
        float texelSizeX = 1.0f / (context.width > 0 ? context.width : 720.0f);
        float texelSizeY = 1.0f / (context.height > 0 ? context.height : 1280.0f);

        // A full modern RHI would have `bindUniformBuffer` instead.
        // To strictly respect our current API, we push a 16 float array and extract in GLESCommandBuffer.
        std::vector<float> beautyParams(16, 0.0f);
        beautyParams[0] = texelSizeX;
        beautyParams[1] = texelSizeY;
        beautyParams[2] = m_smoothing;
        beautyParams[3] = m_whitening;

        context.cmdBuffer->pushConstants(beautyParams.data(), beautyParams.size() * sizeof(float));

        context.cmdBuffer->bindTexture(0, context.inputTexture);
        context.cmdBuffer->drawFullScreenQuad();
    }
}

} // namespace vfx
