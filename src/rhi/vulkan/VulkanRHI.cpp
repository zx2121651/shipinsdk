#include "VulkanRHI.h"
#include <iostream>

namespace vfx {

class VulkanTexture : public ITexture {
public:
    void* getNativeHandle() const override { return nullptr; }
};

class VulkanCommandBuffer : public ICommandBuffer {
public:
    void begin() override {}
    void end() override {}
    void submit() override {}
};

class VulkanPipelineState : public IPipelineState {};

bool VulkanRHI::initialize() { return true; }
void VulkanRHI::shutdown() {}
void VulkanRHI::setWindow(void* window) {}
void VulkanRHI::swapBuffers() {}
void VulkanRHI::renderCameraOESTexture(int textureId, const float* transformMatrix) {}

std::shared_ptr<ITexture> VulkanRHI::createTexture(int width, int height) {
    return std::make_shared<VulkanTexture>();
}

std::shared_ptr<ICommandBuffer> VulkanRHI::createCommandBuffer() {
    return std::make_shared<VulkanCommandBuffer>();
}

std::shared_ptr<IPipelineState> VulkanRHI::createPipelineState() {
    return std::make_shared<VulkanPipelineState>();
}

} // namespace vfx
