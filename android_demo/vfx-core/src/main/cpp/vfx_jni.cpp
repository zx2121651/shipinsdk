#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <GLES2/gl2.h>

#include "vfx_engine/core/RenderThread.h"
#include "vfx_engine/core/RenderGraph.h"
#include "vfx_engine/core/filters/OESCameraFilter.h"
#include "vfx_engine/core/filters/GrayscaleFilter.h"
#include "vfx_engine/rhi/RHI.h"
#include "vfx_engine/media/VideoEncoder.h"

#define LOG_TAG "VFX_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static vfx::RenderThread* gRenderThread = nullptr;
static std::shared_ptr<vfx::IRHI> gRHI = nullptr;
static std::shared_ptr<vfx::RenderGraph> gRenderGraph = nullptr;
static std::shared_ptr<vfx::VideoEncoder> gVideoEncoder = nullptr;

static ANativeWindow* gWindow = nullptr;
static int gCameraTextureId = -1;
static int gCameraWidth = 0;
static int gCameraHeight = 0;

static JavaVM* gJvm = nullptr;
static jobject gVfxEngineObj = nullptr;
static bool gThreadAttached = false;
static bool gGraphInitialized = false;

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
        JNIEnv* jniEnv;
        if (gJvm->AttachCurrentThread(&jniEnv, nullptr) == JNI_OK) {
            gThreadAttached = true;
        }

        if (!gRHI) {
            gRHI = vfx::createRHI(vfx::RHIBackend::Auto, caps);
            gRHI->initialize(caps);
            gGraphInitialized = false;

            gRenderGraph = std::make_shared<vfx::RenderGraph>(gRHI);
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
            if (gRHI && gThreadAttached && gVfxEngineObj) {
                gRHI->makeMainWindowCurrent();

                unsigned int textureId = 0;
                glGenTextures(1, &textureId);
                gCameraTextureId = textureId;

                JNIEnv* jniEnv;
                if (gJvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) == JNI_OK) {
                    jclass clazz = jniEnv->GetObjectClass(gVfxEngineObj);
                    jmethodID methodId = jniEnv->GetMethodID(clazz, "onCameraTextureGenerated", "(I)V");
                    if (methodId) jniEnv->CallVoidMethod(gVfxEngineObj, methodId, textureId);
                    jniEnv->DeleteLocalRef(clazz);
                }
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_setCameraTextureSize(JNIEnv* env, jobject obj, jint width, jint height) {
    gCameraWidth = width;
    gCameraHeight = height;
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_notifyCameraFrameAvailable(JNIEnv* env, jobject obj) {
    if (!gRenderThread || gCameraTextureId < 0) return;

    gRenderThread->postTask([]() {
        if (gRHI && gRenderGraph && gWindow && gThreadAttached && gVfxEngineObj) {

            gRHI->makeMainWindowCurrent();

            if (!gGraphInitialized) {
                gRenderGraph->addFilter(std::make_shared<vfx::OESCameraFilter>());
                gRenderGraph->addFilter(std::make_shared<vfx::GrayscaleFilter>());
                gGraphInitialized = true;
            }

            JNIEnv* jniEnv;
            if (gJvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) == JNI_OK) {
                jclass clazz = jniEnv->GetObjectClass(gVfxEngineObj);
                jmethodID methodId = jniEnv->GetMethodID(clazz, "updateCameraTexture", "()[F");
                if (methodId) {
                    jfloatArray matrixObj = (jfloatArray)jniEnv->CallObjectMethod(gVfxEngineObj, methodId);
                    if (matrixObj) {
                        jfloat* matrixBody = jniEnv->GetFloatArrayElements(matrixObj, 0);

                        vfx::RenderContext ctx;
                        ctx.rhi = gRHI;
                        ctx.inputTextureId = gCameraTextureId;
                        ctx.transformMatrix = matrixBody;
                        ctx.width = gCameraWidth > 0 ? gCameraWidth : 1280;
                        ctx.height = gCameraHeight > 0 ? gCameraHeight : 720;

                        if (gWindow) {
                            gRHI->makeMainWindowCurrent();
                            gRenderGraph->execute(ctx);
                            gRHI->swapBuffers();
                        }

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
                jniEnv->DeleteLocalRef(clazz);
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
                LOGI("RenderThread: Starting Video Encoder to %s", path.c_str());
                gVideoEncoder = vfx::VideoEncoder::create();
                int w = gCameraWidth > 0 ? gCameraWidth : 1280;
                int h = gCameraHeight > 0 ? gCameraHeight : 720;
                if (gVideoEncoder->start(path, w, h, codecType)) {
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
                LOGI("RenderThread: Stopping Video Encoder...");
                gRHI->setEncoderWindow(nullptr);
                gVideoEncoder->stop();
                gVideoEncoder = nullptr;
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_destroy(JNIEnv* env, jobject /* this */) {
    LOGI("Destroying VFX Engine...");

    // Pass a copy of the JNIEnv to the RenderThread for clean JVM detachment.
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            if (gRenderGraph) {
                gRenderGraph->clearFilters();
                gRenderGraph = nullptr;
                gGraphInitialized = false;
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

            // Cleanup JNI references safely on the RenderThread
            if (gThreadAttached && gJvm) {
                JNIEnv* jniEnv;
                if (gJvm->GetEnv((void**)&jniEnv, JNI_VERSION_1_6) == JNI_OK) {
                    if (gVfxEngineObj) {
                        jniEnv->DeleteGlobalRef(gVfxEngineObj);
                        gVfxEngineObj = nullptr;
                    }
                }
                gJvm->DetachCurrentThread();
                gThreadAttached = false;
            }
        });

        gRenderThread->stop();
        delete gRenderThread;
        gRenderThread = nullptr;
    } else {
        if (gVfxEngineObj) {
            env->DeleteGlobalRef(gVfxEngineObj);
            gVfxEngineObj = nullptr;
        }
    }
}
