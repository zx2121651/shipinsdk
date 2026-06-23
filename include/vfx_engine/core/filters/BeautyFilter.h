#pragma once
#include "vfx_engine/core/Filter.h"

namespace vfx {

class BeautyFilter : public IFilter {
public:
    ~BeautyFilter() override;

    bool initialize(std::shared_ptr<IRHI> rhi) override;
    void release() override;
    void process(RenderContext& context) override;

    // Setters for beauty parameters (0.0 to 1.0)
    void setSmoothing(float level) { m_smoothing = level; }
    void setWhitening(float level) { m_whitening = level; }

private:
    std::shared_ptr<IRHI> m_rhi;
    std::shared_ptr<IPipelineState> m_pso;

    float m_smoothing = 0.5f;  // 磨皮程度
    float m_whitening = 0.5f;  // 美白程度
};

} // namespace vfx
