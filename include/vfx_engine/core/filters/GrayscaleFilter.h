#pragma once
#include "vfx_engine/core/Filter.h"

namespace vfx {

class GrayscaleFilter : public IFilter {
public:
    GrayscaleFilter() = default;
    ~GrayscaleFilter() override;

    bool initialize(std::shared_ptr<IRHI> rhi) override;
    void release() override;
    void process(RenderContext& context) override;

private:
    std::shared_ptr<IRHI> m_rhi;
    unsigned int m_programId = 0;
};

} // namespace vfx
