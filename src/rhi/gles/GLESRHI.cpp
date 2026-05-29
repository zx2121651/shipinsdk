#include "GLESRHI.h"
#include <iostream>

#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#define LOG_TAG "VFX_GLES"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
// Mock EGL types for standard C++ compilation without Android NDK
#define LOGI(...) std::cout << __VA_ARGS__ << std::endl
#define LOGE(...) std::cerr << __VA_ARGS__ << std::endl
typedef void* EGLDisplay;
typedef void* EGLContext;
typedef void* EGLSurface;
typedef void* EGLConfig;
typedef void* EGLNativeWindowType;
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_SURFACE ((EGLSurface)0)
#define EGL_TRUE 1
#define EGL_FALSE 0
#define EGL_SUCCESS 0x3000
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_SURFACE_TYPE 0x3033
#define EGL_WINDOW_BIT 0x0004
#define EGL_BLUE_SIZE 0x3022
#define EGL_GREEN_SIZE 0x3023
#define EGL_RED_SIZE 0x3024
#define EGL_NONE 0x3038
#define EGL_CONTEXT_CLIENT_VERSION 0x3098

// Mock GLES bits for non-Android builds
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_OPENGL_ES3_BIT 0x0040

bool eglInitialize(EGLDisplay dpy, int* major, int* minor) { return true; }
EGLDisplay eglGetDisplay(void* display_id) { return (EGLDisplay)1; }
bool eglChooseConfig(EGLDisplay dpy, const int* attrib_list, EGLConfig* configs, int config_size, int* num_config) {
    if (num_config) *num_config = 1;
    return true;
}
EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const int* attrib_list) { return (EGLContext)1; }
EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const int* attrib_list) { return (EGLSurface)1; }
bool eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) { return true; }
bool eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) { return true; }
bool eglDestroySurface(EGLDisplay dpy, EGLSurface surface) { return true; }
bool eglDestroyContext(EGLDisplay dpy, EGLContext ctx) { return true; }
bool eglTerminate(EGLDisplay dpy) { return true; }
void glClearColor(float r, float g, float b, float a) {}
void glClear(unsigned int mask) {}
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif

namespace vfx {

class GLESTexture : public ITexture {
public:
    void* getNativeHandle() const override { return nullptr; }
};

class GLESCommandBuffer : public ICommandBuffer {
public:
    void begin() override {}
    void end() override {}
    void submit() override {}
};

class GLESPipelineState : public IPipelineState {};

GLESRHI::GLESRHI()
    : m_eglDisplay(nullptr),
      m_eglContext(nullptr),
      m_eglSurface(nullptr),
      m_eglConfig(nullptr) {
}

GLESRHI::~GLESRHI() {
    shutdown();
}

bool GLESRHI::initialize() {
    LOGI("Initializing GLES RHI (EGL Context)...");

    EGLDisplay display = eglGetDisplay(nullptr); // EGL_DEFAULT_DISPLAY
    if (display == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed.");
        return false;
    }

    if (eglInitialize(display, nullptr, nullptr) != EGL_TRUE) {
        LOGE("eglInitialize failed.");
        return false;
    }

    // Attempt GLES 3.0 first
    int attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    int numConfigs;
    bool isGLES3 = true;

    if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs == 0) {
        LOGI("GLES 3.0 config not supported, falling back to GLES 2.0...");

        // Fallback to GLES 2.0
        attribs[1] = EGL_OPENGL_ES2_BIT;
        isGLES3 = false;

        if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs == 0) {
            LOGE("eglChooseConfig failed for both GLES 3.0 and GLES 2.0.");
            return false;
        }
    }

    int contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, isGLES3 ? 3 : 2,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed.");
        return false;
    }

    m_eglDisplay = display;
    m_eglContext = context;
    m_eglConfig = config;

    if (isGLES3) {
        LOGI("Successfully initialized EGL Context for GLES 3.0.");
    } else {
        LOGI("Successfully initialized EGL Context for GLES 2.0.");
    }

    return true;
}

void GLESRHI::shutdown() {
    LOGI("Shutting down GLES RHI...");

    EGLDisplay display = (EGLDisplay)m_eglDisplay;
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (m_eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(display, (EGLSurface)m_eglSurface);
            m_eglSurface = nullptr;
        }

        if (m_eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(display, (EGLContext)m_eglContext);
            m_eglContext = nullptr;
        }

        eglTerminate(display);
        m_eglDisplay = nullptr;
    }
}

void GLESRHI::setWindow(void* window) {
    EGLDisplay display = (EGLDisplay)m_eglDisplay;

    if (m_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(display, (EGLSurface)m_eglSurface);
        m_eglSurface = nullptr;
    }

    if (window != nullptr) {
#ifdef __ANDROID__
        EGLNativeWindowType nativeWindow = static_cast<EGLNativeWindowType>(window);
#else
        void* nativeWindow = window;
#endif
        m_eglSurface = eglCreateWindowSurface(display, (EGLConfig)m_eglConfig, nativeWindow, nullptr);

        if (m_eglSurface == EGL_NO_SURFACE) {
            LOGE("eglCreateWindowSurface failed.");
            return;
        }

        if (eglMakeCurrent(display, (EGLSurface)m_eglSurface, (EGLSurface)m_eglSurface, (EGLContext)m_eglContext) != EGL_TRUE) {
            LOGE("eglMakeCurrent failed.");
        }

        // Demo clear color setup
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
    } else {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

void GLESRHI::swapBuffers() {
    if (m_eglDisplay != EGL_NO_DISPLAY && m_eglSurface != EGL_NO_SURFACE) {
        glClear(GL_COLOR_BUFFER_BIT); // Test render
        eglSwapBuffers((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurface);
    }
}

std::shared_ptr<ITexture> GLESRHI::createTexture(int width, int height) {
    return std::make_shared<GLESTexture>();
}

std::shared_ptr<ICommandBuffer> GLESRHI::createCommandBuffer() {
    return std::make_shared<GLESCommandBuffer>();
}

std::shared_ptr<IPipelineState> GLESRHI::createPipelineState() {
    return std::make_shared<GLESPipelineState>();
}

} // namespace vfx
