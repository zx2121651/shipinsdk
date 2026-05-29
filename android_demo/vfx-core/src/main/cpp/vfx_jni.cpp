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

// Camera feed tracking
static int gCameraTextureId = -1;
static int gCameraWidth = 0;
static int gCameraHeight = 0;

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_init(JNIEnv* env, jobject /* this */) {
    LOGI("Initializing VFX Engine...");

    if (!gRenderThread) {
        gRenderThread = new vfx::RenderThread();
        gRenderThread->start();
    }

    gRenderThread->postTask([]() {
        if (!gRHI) {
            // EGL context logic needs to run on the RenderThread
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
                // Bind window in RenderThread (makes EGL Context current with surface)
                gRHI->setWindow(gWindow);

                if (gWindow) {
                    // Force an initial clear frame
                    auto cmd = gRHI->createCommandBuffer();
                    cmd->begin();
                    gRHI->swapBuffers();
                    cmd->end();
                    cmd->submit();
                }
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_setCameraTexture(JNIEnv* env, jobject /* this */, jint textureId, jint width, jint height) {
    LOGI("Camera texture registered ID: %d, Size: %dx%d", textureId, width, height);
    gCameraTextureId = textureId;
    gCameraWidth = width;
    gCameraHeight = height;

    if (gRenderThread) {
        gRenderThread->postTask([textureId, width, height]() {
            // In a real pipeline, the engine will create an OES ITexture representation
            // wrapping this textureId and bind it to the shader pipeline.
            LOGI("RenderThread: Ready to render Camera Texture %d", textureId);
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_notifyCameraFrameAvailable(JNIEnv* env, jobject /* this */) {
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            if (gRHI && gWindow) {
                // 1. (Omitted) eglMakeCurrent
                // 2. (Omitted) Update surface texture using OpenGL specific extension
                // 3. (Omitted) Bind Pipeline and draw the camera OES texture to the screen buffer

                auto cmd = gRHI->createCommandBuffer();
                cmd->begin();
                gRHI->swapBuffers();
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
            LOGI("RenderThread: Setup video encoder for %s", path.c_str());
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_stopRecording(JNIEnv* env, jobject /* this */) {
    LOGI("Stopping recording...");
    if (gRenderThread) {
        gRenderThread->postTask([]() {
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
                gRHI->setWindow(nullptr);
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
