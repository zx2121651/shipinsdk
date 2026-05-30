#include "vfx_engine/rhi/RHI.h"
#include "vulkan/VulkanRHI.h"
#include "metal/MetalRHI.h"
#include "gles/GLESRHI.h"

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "VFX_RHI_FACTORY"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <iostream>
#define LOGI(...) do {} while(0)
#define LOGE(...) do {} while(0)
#endif

namespace vfx {

std::shared_ptr<IRHI> createRHI(RHIBackend backend, const HardwareCapabilities& caps) {
    if (backend == RHIBackend::Auto) {
#ifdef __APPLE__
        LOGI("Auto-detecting RHI: Selecting Metal for Apple platform.");
        backend = RHIBackend::Metal;
#elif defined(__ANDROID__)
        if (caps.isVulkanSupported) {
            LOGI("Auto-detecting RHI: Vulkan is reported as supported by OS.");
            // Falling back to GLES safely since VulkanRHI is a stub
            backend = RHIBackend::GLES;
        } else {
            LOGI("Auto-detecting RHI: Vulkan not supported. Falling back to GLES.");
            backend = RHIBackend::GLES;
        }
#else
        LOGI("Auto-detecting RHI: Defaulting to GLES for generic platform.");
        backend = RHIBackend::GLES;
#endif
    }

    switch (backend) {
        case RHIBackend::Vulkan:
            LOGI("Creating Vulkan RHI Backend");
            return std::make_shared<VulkanRHI>();
        case RHIBackend::Metal:
            LOGI("Creating Metal RHI Backend");
            return std::make_shared<MetalRHI>();
        case RHIBackend::GLES:
            LOGI("Creating GLES RHI Backend");
            return std::make_shared<GLESRHI>();
        default:
            LOGE("Unknown RHI Backend requested.");
            return nullptr;
    }
}

} // namespace vfx
