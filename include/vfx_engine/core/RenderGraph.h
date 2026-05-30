#pragma once

#include <vector>
#include <memory>
#include "vfx_engine/core/Filter.h"
#include "vfx_engine/rhi/RHI.h"

namespace vfx {

class RenderGraph {
public:
    RenderGraph(std::shared_ptr<IRHI> rhi);
    ~RenderGraph();

    void addFilter(std::shared_ptr<IFilter> filter);
    void clearFilters();

    // Executes all filters in sequence
    void execute(RenderContext& initialContext);

private:
    std::shared_ptr<IRHI> m_rhi;
    std::vector<std::shared_ptr<IFilter>> m_filters;
};

} // namespace vfx
