#include "vfx_engine/rhi/RHI.h"
#include "vulkan/VulkanRHI.h"
#include "metal/MetalRHI.h"
#include "gles/GLESRHI.h"

#ifdef __ANDROID__
#include <dlfcn.h>
#include <android/log.h>
#define LOG_TAG "VFX_RHI_FACTORY"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <iostream>
#define LOGI(...) std::cout << __VA_ARGS__ << std::endl
#define LOGE(...) std::cerr << __VA_ARGS__ << std::endl
#endif

namespace vfx {

static bool isVulkanSupportedOnAndroid() {
#ifdef __ANDROID__
    // Dynamically attempt to load libvulkan.so to verify device support
    void* libvulkan = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    if (!libvulkan) {
        LOGI("libvulkan.so not found on device.");
        return false;
    }

    // Check if vkCreateInstance is available as a proxy for basic support
    void* vkCreateInstance = dlsym(libvulkan, "vkCreateInstance");
    dlclose(libvulkan);

    if (vkCreateInstance != nullptr) {
        LOGI("Vulkan API detected on device.");
        return true;
    } else {
        LOGI("libvulkan.so found but missing vkCreateInstance (Invalid/Stub).");
        return false;
    }
#else
    // For standard desktop builds, we assume Vulkan isn't natively the default fallback
    // unless explicitly handled by something like GLFW/SDL.
    return false;
#endif
}

std::shared_ptr<IRHI> createRHI(RHIBackend backend) {

    if (backend == RHIBackend::Auto) {
#ifdef __APPLE__
        LOGI("Auto-detecting RHI: Selecting Metal for Apple platform.");
        backend = RHIBackend::Metal;
#elif defined(__ANDROID__)
        if (isVulkanSupportedOnAndroid()) {
            LOGI("Auto-detecting RHI: Vulkan is supported. (Note: Vulkan RHI currently stubbed, falling back to GLES for safe execution)");
            // In a production engine, this would be RHIBackend::Vulkan
            // Since our VulkanRHI is a stub right now, we safely fallback to GLES to avoid black screens.
            backend = RHIBackend::GLES;
        } else {
            LOGI("Auto-detecting RHI: Vulkan not supported. Falling back to GLES.");
            backend = RHIBackend::GLES;
        }
#else
        LOGI("Auto-detecting RHI: Defaulting to Vulkan/GLES for generic platform.");
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
