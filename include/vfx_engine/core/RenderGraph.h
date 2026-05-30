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

    // Reallocates FBOs if viewport size changes
    void resize(int width, int height);

    // Executes all filters in sequence using a ping-pong FBO strategy
    void execute(RenderContext& initialContext);

private:
    std::shared_ptr<IRHI> m_rhi;
    std::vector<std::shared_ptr<IFilter>> m_filters;

    // Ping-pong render targets for chaining passes
    std::shared_ptr<IRenderTarget> m_fboA;
    std::shared_ptr<IRenderTarget> m_fboB;
    int m_width = 0;
    int m_height = 0;
};

} // namespace vfx
