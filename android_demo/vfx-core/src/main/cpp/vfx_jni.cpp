#include <jni.h>
#include <string>
#include <vector>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <GLES2/gl2.h>

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
    } else {
        LOGI("Surface un-bound from VFX Engine.");
    }

    if (gRenderThread) {
        gRenderThread->postTask([newWindow]() {
            if (gRHI) {
                gRHI->setWindow(newWindow);

                if (gWindow) {
                    ANativeWindow_release(gWindow);
                }
                gWindow = newWindow;

                if (gWindow) {
                    auto cmd = gRHI->createCommandBuffer();
                    cmd->begin();
                    gRHI->swapBuffers();
                    cmd->end();
                    cmd->submit();
                }
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
                // Generate a real OpenGL texture ID inside the EGL context thread
                unsigned int textureId;
                glGenTextures(1, &textureId);
                gCameraTextureId = textureId;

                LOGI("Generated real Camera OES Texture ID: %d", textureId);

                // Call back to Kotlin
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
        if (gRHI && gWindow) {
            // updateTexImage and getTransformMatrix must be called ON the EGL context thread
            JNIEnv* jniEnv;
            if (gJvm->AttachCurrentThread(&jniEnv, nullptr) == JNI_OK) {
                jclass clazz = jniEnv->GetObjectClass(gVfxEngineObj);
                jmethodID methodId = jniEnv->GetMethodID(clazz, "updateCameraTexture", "()[F");
                if (methodId) {
                    jfloatArray matrixObj = (jfloatArray)jniEnv->CallObjectMethod(gVfxEngineObj, methodId);
                    if (matrixObj) {
                        jfloat* matrixBody = jniEnv->GetFloatArrayElements(matrixObj, 0);

                        gRHI->renderCameraOESTexture(gCameraTextureId, matrixBody);

                        auto cmd = gRHI->createCommandBuffer();
                        cmd->begin();
                        gRHI->swapBuffers();
                        cmd->end();
                        cmd->submit();

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
            LOGI("RenderThread: Setup video encoder for %s", path.c_str());
        });
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_vfx_core_VfxEngine_stopRecording(JNIEnv* env, jobject /* this */) {
    if (gRenderThread) {
        gRenderThread->postTask([]() {
            LOGI("RenderThread: Finalizing video export...");
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
            if (gRHI) {
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
