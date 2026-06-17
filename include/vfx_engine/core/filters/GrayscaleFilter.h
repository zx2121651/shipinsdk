#pragma once
#include "vfx_engine/core/Filter.h"

namespace vfx {

class GrayscaleFilter : public IFilter {
public:
    ~GrayscaleFilter() override;

    bool initialize(std::shared_ptr<IRHI> rhi) override;
    void release() override;
    void process(RenderContext& context) override;

private:
    std::shared_ptr<IRHI> m_rhi;
    std::shared_ptr<IPipelineState> m_pso;
};

} // namespace vfx
