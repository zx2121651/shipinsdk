#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>

#include "vfx_engine/core/RenderThread.h"
#include "vfx_engine/rhi/RHI.h"

#define LOG_TAG "VFX_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static vfx::RenderThread* gRenderThread = nullptr;
static std::shared_ptr<vfx::IRHI> gRHI = nullptr;
static ANativeWindow* gWindow = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_init(JNIEnv* env, jobject /* this */) {
    LOGI("Initializing VFX Engine...");

    if (!gRenderThread) {
        gRenderThread = new vfx::RenderThread();
        gRenderThread->start();
    }

    gRenderThread->postTask([]() {
        if (!gRHI) {
            gRHI = vfx::createRHI(vfx::RHIBackend::GLES);
            gRHI->initialize();
        }
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_setSurface(JNIEnv* env, jobject /* this */, jobject surface) {
    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }
    if (surface) {
        gWindow = ANativeWindow_fromSurface(env, surface);
        LOGI("Surface bound to VFX Engine.");
    } else {
        LOGI("Surface un-bound from VFX Engine.");
    }

    if (gRenderThread) {
        gRenderThread->postTask([]() {
            if (gRHI) {
                // Simulate frame rendering update on the RenderThread
                auto cmd = gRHI->createCommandBuffer();
                cmd->begin();
                cmd->end();
                cmd->submit();
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_startRecording(JNIEnv* env, jobject /* this */, jstring outputPath) {
    const char *nativeString = env->GetStringUTFChars(outputPath, 0);
    std::string path(nativeString);
    env->ReleaseStringUTFChars(outputPath, nativeString);

    LOGI("Starting recording to %s", path.c_str());

    if (gRenderThread) {
        gRenderThread->postTask([path]() {
            // Placeholder: Initialize video encoder using Media3 / MediaCodec
            LOGI("RenderThread: Setup video encoder for %s", path.c_str());
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_stopRecording(JNIEnv* env, jobject /* this */) {
    LOGI("Stopping recording...");
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            // Placeholder: Finalize video file and export
            LOGI("RenderThread: Finalizing video export...");
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_destroy(JNIEnv* env, jobject /* this */) {
    LOGI("Destroying VFX Engine...");
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            if (gRHI) {
                gRHI->shutdown();
                gRHI = nullptr;
            }
        });
        gRenderThread->stop();
        delete gRenderThread;
        gRenderThread = nullptr;
    }
    if (gWindow) {
        ANativeWindow_release(gWindow);
        gWindow = nullptr;
    }
}
