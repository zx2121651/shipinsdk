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
    if (m_filters.empty() || !m_rhi || !initialContext.cmdBuffer) return;

    // Make sure FBO pool is sized correctly
    resize(initialContext.width, initialContext.height);

    RenderContext currentContext = initialContext;
    currentContext.rhi = m_rhi;

    std::shared_ptr<IRenderTarget> sourceFbo = m_fboA;
    std::shared_ptr<IRenderTarget> destFbo = m_fboB;

    // The entire graph execution is recorded into the provided CommandBuffer
    initialContext.cmdBuffer->begin();

    for (size_t i = 0; i < m_filters.size(); ++i) {
        bool isLastFilter = (i == m_filters.size() - 1);

        RenderPassDescriptor passDesc;
        // Pass down whether we are targeting the screen or the media encoder surface.
        // In a pure RHI, this would be determined by the specific swapchain/surface bound to the RenderTarget.
        passDesc.isEncoderTarget = initialContext.isEncoderTarget;

        if (!isLastFilter) {
            // Render to offscreen FBO
            passDesc.colorAttachment = destFbo;
        } else {
            // Render directly to default window surface
            passDesc.colorAttachment = nullptr;
        }

        passDesc.clearColor = true;
        initialContext.cmdBuffer->beginRenderPass(passDesc);

        m_filters[i]->process(currentContext);

        initialContext.cmdBuffer->endRenderPass();

        if (!isLastFilter) {
            // The output of this pass becomes the input of the next pass
            currentContext.inputTexture = destFbo->getTexture();
            currentContext.transformMatrix = nullptr; // Reset OES matrix for subsequent 2D passes

            // Ping-pong targets
            std::swap(sourceFbo, destFbo);
        }
    }

    initialContext.cmdBuffer->end();

    // Actually submit the recorded work to the GPU
    initialContext.cmdBuffer->submit();
}

} // namespace vfx
