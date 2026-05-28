#include "vfx_engine/rhi/RHI.h"
#include "vulkan/VulkanRHI.h"
#include "metal/MetalRHI.h"
#include "gles/GLESRHI.h"

namespace vfx {

std::shared_ptr<IRHI> createRHI(RHIBackend backend) {
    switch (backend) {
        case RHIBackend::Vulkan:
            return std::make_shared<VulkanRHI>();
        case RHIBackend::Metal:
            return std::make_shared<MetalRHI>();
        case RHIBackend::GLES:
            return std::make_shared<GLESRHI>();
        default:
            return nullptr;
    }
}

} // namespace vfx
