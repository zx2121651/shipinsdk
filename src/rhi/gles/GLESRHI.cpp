#include "GLESRHI.h"
#include <iostream>

namespace vfx {

class GLESTexture : public ITexture {
public:
    void* getNativeHandle() const override { return nullptr; }
};

class GLESCommandBuffer : public ICommandBuffer {
public:
    void begin() override {}
    void end() override {}
    void submit() override {}
};

class GLESPipelineState : public IPipelineState {};


bool GLESRHI::initialize() {
    std::cout << "Initializing GLES RHI..." << std::endl;
    return true;
}

void GLESRHI::shutdown() {
    std::cout << "Shutting down GLES RHI..." << std::endl;
}

std::shared_ptr<ITexture> GLESRHI::createTexture(int width, int height) {
    return std::make_shared<GLESTexture>();
}

std::shared_ptr<ICommandBuffer> GLESRHI::createCommandBuffer() {
    return std::make_shared<GLESCommandBuffer>();
}

std::shared_ptr<IPipelineState> GLESRHI::createPipelineState() {
    return std::make_shared<GLESPipelineState>();
}

} // namespace vfx
