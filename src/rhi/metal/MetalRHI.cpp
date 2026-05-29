#include "MetalRHI.h"
#include <iostream>

namespace vfx {

class MetalTexture : public ITexture {
public:
    void* getNativeHandle() const override { return nullptr; }
};

class MetalCommandBuffer : public ICommandBuffer {
public:
    void begin() override {}
    void end() override {}
    void submit() override {}
};

class MetalPipelineState : public IPipelineState {};


bool MetalRHI::initialize() {
    std::cout << "Initializing Metal RHI..." << std::endl;
    return true;
}

void MetalRHI::shutdown() {
    std::cout << "Shutting down Metal RHI..." << std::endl;
}

void MetalRHI::setWindow(void* window) {
}

void MetalRHI::swapBuffers() {
}

std::shared_ptr<ITexture> MetalRHI::createTexture(int width, int height) {
    return std::make_shared<MetalTexture>();
}

std::shared_ptr<ICommandBuffer> MetalRHI::createCommandBuffer() {
    return std::make_shared<MetalCommandBuffer>();
}

std::shared_ptr<IPipelineState> MetalRHI::createPipelineState() {
    return std::make_shared<MetalPipelineState>();
}

} // namespace vfx
