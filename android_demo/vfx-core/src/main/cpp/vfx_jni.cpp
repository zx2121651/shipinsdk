#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <GLES2/gl2.h>

#include "vfx_engine/core/RenderThread.h"
#include "vfx_engine/rhi/RHI.h"
#include "vfx_engine/media/VideoEncoder.h"

#define LOG_TAG "VFX_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static vfx::RenderThread* gRenderThread = nullptr;
static std::shared_ptr<vfx::IRHI> gRHI = nullptr;
static std::shared_ptr<vfx::VideoEncoder> gVideoEncoder = nullptr;

static ANativeWindow* gWindow = nullptr;

// Camera feed tracking
static int gCameraTextureId = -1;

// To store reference to JVM to trigger callbacks
static JavaVM* gJvm = nullptr;
static jobject gVfxEngineObj = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_init(JNIEnv* env, jobject obj) {
    LOGI("Initializing VFX Engine...");

    env->GetJavaVM(&gJvm);
    if(gVfxEngineObj) {
        env->DeleteGlobalRef(gVfxEngineObj);
    }
    gVfxEngineObj = env->NewGlobalRef(obj);

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
    ANativeWindow* newWindow = nullptr;
    if (surface) {
        newWindow = ANativeWindow_fromSurface(env, surface);
        LOGI("Surface bound to VFX Engine.");
    }

    if (gRenderThread) {
        gRenderThread->postTask([newWindow]() {
            if (gRHI) {
                gRHI->setWindow(newWindow);

                if (gWindow) {
                    ANativeWindow_release(gWindow);
                }
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
            if (gRHI) {
                unsigned int textureId;
                glGenTextures(1, &textureId);
                gCameraTextureId = textureId;

                JNIEnv* env;
                if (gJvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
                    jclass clazz = env->GetObjectClass(gVfxEngineObj);
                    jmethodID methodId = env->GetMethodID(clazz, "onCameraTextureGenerated", "(I)V");
                    if (methodId) {
                        env->CallVoidMethod(gVfxEngineObj, methodId, textureId);
                    }
                    gJvm->DetachCurrentThread();
                }
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_notifyCameraFrameAvailable(JNIEnv* env, jobject obj) {
    if (!gRenderThread || gCameraTextureId < 0) return;

    gRenderThread->postTask([]() {
        if (gRHI) {
            JNIEnv* jniEnv;
            if (gJvm->AttachCurrentThread(&jniEnv, nullptr) == JNI_OK) {
                jclass clazz = jniEnv->GetObjectClass(gVfxEngineObj);
                jmethodID methodId = jniEnv->GetMethodID(clazz, "updateCameraTexture", "()[F");
                if (methodId) {
                    jfloatArray matrixObj = (jfloatArray)jniEnv->CallObjectMethod(gVfxEngineObj, methodId);
                    if (matrixObj) {
                        jfloat* matrixBody = jniEnv->GetFloatArrayElements(matrixObj, 0);

                        // 1. Draw to Main Preview Window
                        if (gWindow) {
                            gRHI->makeMainWindowCurrent();
                            gRHI->renderCameraOESTexture(gCameraTextureId, matrixBody);
                            gRHI->swapBuffers();
                        }

                        // 2. Draw to Encoder Window (If recording)
                        if (gVideoEncoder) {
                            gRHI->makeEncoderWindowCurrent();
                            gRHI->renderCameraOESTexture(gCameraTextureId, matrixBody);
                            gVideoEncoder->notifyFrameReady(); // Tells encoder to grab pts and drain
                            gRHI->swapEncoderBuffers();
                        }

                        jniEnv->ReleaseFloatArrayElements(matrixObj, matrixBody, 0);
                        jniEnv->DeleteLocalRef(matrixObj);
                    }
                }
                gJvm->DetachCurrentThread();
            }
        }
    });
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_startRecording(JNIEnv* env, jobject /* this */, jstring outputPath) {
    const char *nativeString = env->GetStringUTFChars(outputPath, 0);
    std::string path(nativeString);
    env->ReleaseStringUTFChars(outputPath, nativeString);

    if (gRenderThread) {
        gRenderThread->postTask([path]() {
            if (!gVideoEncoder && gRHI) {
                LOGI("RenderThread: Starting Video Encoder to %s", path.c_str());
                gVideoEncoder = vfx::VideoEncoder::create();

                // Typical HD camera resolution for testing
                if (gVideoEncoder->start(path, 1280, 720)) {
                    void* encoderWindow = gVideoEncoder->getInputWindow();
                    gRHI->setEncoderWindow(encoderWindow);
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
                gRHI->setEncoderWindow(nullptr); // Unbind encoder surface
                gVideoEncoder->stop();
                gVideoEncoder = nullptr;
            }
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_destroy(JNIEnv* env, jobject /* this */) {
    LOGI("Destroying VFX Engine...");

    if (gVfxEngineObj) {
        env->DeleteGlobalRef(gVfxEngineObj);
        gVfxEngineObj = nullptr;
    }

    if (gRenderThread) {
        gRenderThread->postTask([]() {
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
        });
        gRenderThread->stop();
        delete gRenderThread;
        gRenderThread = nullptr;
    }
}
