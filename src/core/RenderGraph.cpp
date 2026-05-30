#include "vfx_engine/core/RenderGraph.h"
#include <iostream>

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

void RenderGraph::resize(int width, int height) {
    if (m_width == width && m_height == height) return;

    m_width = width;
    m_height = height;

    if (m_rhi) {
        m_fboA = m_rhi->createRenderTarget(width, height);
        m_fboB = m_rhi->createRenderTarget(width, height);
    }
}

void RenderGraph::execute(RenderContext& initialContext) {
    if (m_filters.empty() || !m_rhi) return;

    // Make sure FBO pool is sized correctly
    resize(initialContext.width, initialContext.height);

    RenderContext currentContext = initialContext;
    currentContext.rhi = m_rhi;

    std::shared_ptr<IRenderTarget> sourceFbo = m_fboA;
    std::shared_ptr<IRenderTarget> destFbo = m_fboB;

    for (size_t i = 0; i < m_filters.size(); ++i) {
        bool isLastFilter = (i == m_filters.size() - 1);

        if (!isLastFilter) {
            // Render to offscreen FBO
            m_rhi->bindRenderTarget(destFbo);
            currentContext.outputTextureId = destFbo->getTextureId();
        } else {
            // Render directly to default window surface
            m_rhi->unbindRenderTarget();
            currentContext.outputTextureId = -1; // -1 indicates default surface
        }

        m_filters[i]->process(currentContext);

        if (!isLastFilter) {
            // The output of this pass becomes the input of the next pass
            currentContext.inputTextureId = currentContext.outputTextureId;
            currentContext.transformMatrix = nullptr; // Reset OES matrix for subsequent 2D passes

            // Ping-pong targets
            std::swap(sourceFbo, destFbo);
        }
    }
}

} // namespace vfx
