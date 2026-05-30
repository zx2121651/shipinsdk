#include "vfx_engine/core/RenderGraph.h"

namespace vfx {

RenderGraph::RenderGraph(std::shared_ptr<IRHI> rhi) : m_rhi(rhi) {
}

RenderGraph::~RenderGraph() {
    clearFilters();
}

void RenderGraph::addFilter(std::shared_ptr<IFilter> filter) {
    if (filter && filter->initialize(m_rhi)) {
        m_filters.push_back(filter);
    }
}

void RenderGraph::clearFilters() {
    for (auto& filter : m_filters) {
        filter->release();
    }
    m_filters.clear();
}

void RenderGraph::execute(RenderContext& initialContext) {
    RenderContext currentContext = initialContext;
    currentContext.rhi = m_rhi;

    for (size_t i = 0; i < m_filters.size(); ++i) {
        // In a true FBO setup, the output of filter i becomes the input of filter i+1
        m_filters[i]->process(currentContext);

        // Advance pipeline (For now, we just pass the same context along)
        // In the future: currentContext.inputTextureId = currentContext.outputTextureId;
    }
}

} // namespace vfx
