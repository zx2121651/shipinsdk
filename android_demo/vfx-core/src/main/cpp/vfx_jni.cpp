#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <GLES2/gl2.h>

#include "vfx_engine/core/RenderThread.h"
#include "vfx_engine/core/RenderGraph.h"
#include "vfx_engine/core/filters/OESCameraFilter.h"
#include "vfx_engine/rhi/RHI.h"
#include "vfx_engine/media/VideoEncoder.h"

#define LOG_TAG "VFX_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static vfx::RenderThread* gRenderThread = nullptr;
static std::shared_ptr<vfx::IRHI> gRHI = nullptr;
static std::shared_ptr<vfx::RenderGraph> gRenderGraph = nullptr;
static std::shared_ptr<vfx::VideoEncoder> gVideoEncoder = nullptr;

static ANativeWindow* gWindow = nullptr;
static int gCameraTextureId = -1;

static JavaVM* gJvm = nullptr;
static jobject gVfxEngineObj = nullptr;
static bool gThreadAttached = false;

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_init(JNIEnv* env, jobject obj, jint glesVersionHex, jboolean isVulkanSupported) {
    LOGI("Initializing VFX Engine...");
    env->GetJavaVM(&gJvm);
    if(gVfxEngineObj) env->DeleteGlobalRef(gVfxEngineObj);
    gVfxEngineObj = env->NewGlobalRef(obj);

    if (!gRenderThread) {
        gRenderThread = new vfx::RenderThread();
        gRenderThread->start();
    }

    vfx::HardwareCapabilities caps;
    caps.glesVersionHex = glesVersionHex;
    caps.isVulkanSupported = isVulkanSupported;

    gRenderThread->postTask([caps]() {
        // Attach the RenderThread to the JVM ONCE.
        JNIEnv* jniEnv;
        if (gJvm->AttachCurrentThread(&jniEnv, nullptr) == JNI_OK) {
            gThreadAttached = true;
        }

        if (!gRHI) {
            gRHI = vfx::createRHI(vfx::RHIBackend::Auto, caps);
            gRHI->initialize(caps);

            // RenderGraph is disabled for multi-pass until FBOs are implemented.
            // Using a single OES filter for now to render the camera frame.
            gRenderGraph = std::make_shared<vfx::RenderGraph>(gRHI);
            gRenderGraph->addFilter(std::make_shared<vfx::OESCameraFilter>());
        }
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_setSurface(JNIEnv* env, jobject /* this */, jobject surface) {
    ANativeWindow* newWindow = nullptr;
    if (surface) newWindow = ANativeWindow_fromSurface(env, surface);

    if (gRenderThread) {
        gRenderThread->postTask([newWindow]() {
            if (gRHI) {
                gRHI->setWindow(newWindow);
                if (gWindow) ANativeWindow_release(gWindow);
                gWindow = newWindow;
            }
        });
    } else {
        if (newWindow) ANativeWindow_release(newWindow);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_generateCameraTexture(JNIEnv* env, jobject obj) {
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            if (gRHI && gThreadAttached) {
                // IMPORTANT: Must make a context current before calling GL commands!
                // If there's no main window yet, this requires an offscreen PBuffer,
                // but since setSurface runs before this in Android lifecycle, we enforce it here:
                gRHI->makeMainWindowCurrent();

                unsigned int textureId = 0;
                glGenTextures(1, &textureId);
                gCameraTextureId = textureId;

                JNIEnv* jniEnv;
                if (gJvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) == JNI_OK) {
                    jclass clazz = jniEnv->GetObjectClass(gVfxEngineObj);
                    jmethodID methodId = jniEnv->GetMethodID(clazz, "onCameraTextureGenerated", "(I)V");
                    if (methodId) jniEnv->CallVoidMethod(gVfxEngineObj, methodId, textureId);
                }
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_notifyCameraFrameAvailable(JNIEnv* env, jobject obj) {
    if (!gRenderThread || gCameraTextureId < 0) return;

    gRenderThread->postTask([]() {
        if (gRHI && gRenderGraph && gWindow && gThreadAttached) {

            // 1. MUST MAKE CONTEXT CURRENT BEFORE updateTexImage()
            gRHI->makeMainWindowCurrent();

            JNIEnv* jniEnv;
            if (gJvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) == JNI_OK) {
                jclass clazz = jniEnv->GetObjectClass(gVfxEngineObj);
                jmethodID methodId = jniEnv->GetMethodID(clazz, "updateCameraTexture", "()[F");
                if (methodId) {
                    // This calls SurfaceTexture.updateTexImage() safely now.
                    jfloatArray matrixObj = (jfloatArray)jniEnv->CallObjectMethod(gVfxEngineObj, methodId);
                    if (matrixObj) {
                        jfloat* matrixBody = jniEnv->GetFloatArrayElements(matrixObj, 0);

                        vfx::RenderContext ctx;
                        ctx.rhi = gRHI;
                        ctx.inputTextureId = gCameraTextureId;
                        ctx.transformMatrix = matrixBody;

                        // Execute Pipeline on Main UI Window
                        gRenderGraph->execute(ctx);
                        gRHI->swapBuffers();

                        // Execute Pipeline on Encoder Window
                        if (gVideoEncoder) {
                            gRHI->makeEncoderWindowCurrent();
                            gRenderGraph->execute(ctx);
                            gVideoEncoder->notifyFrameReady();
                            gRHI->swapEncoderBuffers();
                        }

                        jniEnv->ReleaseFloatArrayElements(matrixObj, matrixBody, 0);
                        jniEnv->DeleteLocalRef(matrixObj);
                    }
                }
            }
        }
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_startRecording(JNIEnv* env, jobject /* this */, jstring outputPath, jint codecTypeInt) {
    const char *nativeString = env->GetStringUTFChars(outputPath, 0);
    std::string path(nativeString);
    env->ReleaseStringUTFChars(outputPath, nativeString);
    vfx::VideoCodecType codecType = (codecTypeInt == 1) ? vfx::VideoCodecType::H265 : vfx::VideoCodecType::H264;

    if (gRenderThread) {
        gRenderThread->postTask([path, codecType]() {
            if (!gVideoEncoder && gRHI) {
                gVideoEncoder = vfx::VideoEncoder::create();
                if (gVideoEncoder->start(path, 1280, 720, codecType)) {
                    gRHI->setEncoderWindow(gVideoEncoder->getInputWindow());
                } else {
                    gVideoEncoder = nullptr;
                }
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_stopRecording(JNIEnv* env, jobject /* this */) {
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            if (gVideoEncoder && gRHI) {
                gRHI->setEncoderWindow(nullptr);
                gVideoEncoder->stop();
                gVideoEncoder = nullptr;
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_destroy(JNIEnv* env, jobject /* this */) {
    if (gVfxEngineObj) {
        env->DeleteGlobalRef(gVfxEngineObj);
        gVfxEngineObj = nullptr;
    }
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            if (gRenderGraph) {
                gRenderGraph->clearFilters();
                gRenderGraph = nullptr;
            }
            if (gVideoEncoder) {
                gVideoEncoder->stop();
                gVideoEncoder = nullptr;
            }
            if (gRHI) {
                gRHI->setEncoderWindow(nullptr);
                gRHI->setWindow(nullptr);
                gRHI->shutdown();
                gRHI = nullptr;
            }
            if (gWindow) {
                ANativeWindow_release(gWindow);
                gWindow = nullptr;
            }

            if (gThreadAttached && gJvm) {
                gJvm->DetachCurrentThread();
                gThreadAttached = false;
            }
        });
        gRenderThread->stop();
        delete gRenderThread;
        gRenderThread = nullptr;
    }
}
