#include "GLESRHI.h"
#include <iostream>

#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#else
// Mock EGL types for standard C++ compilation without Android NDK
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

bool eglInitialize(EGLDisplay dpy, int* major, int* minor) { return true; }
EGLDisplay eglGetDisplay(void* display_id) { return (EGLDisplay)1; }
bool eglChooseConfig(EGLDisplay dpy, const int* attrib_list, EGLConfig* configs, int config_size, int* num_config) { return true; }
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
    std::cout << "Initializing GLES RHI (EGL Context)..." << std::endl;

    EGLDisplay display = eglGetDisplay(nullptr); // EGL_DEFAULT_DISPLAY
    if (display == EGL_NO_DISPLAY) {
        std::cerr << "eglGetDisplay failed." << std::endl;
        return false;
    }

    if (eglInitialize(display, nullptr, nullptr) != EGL_TRUE) {
        std::cerr << "eglInitialize failed." << std::endl;
        return false;
    }

#ifdef __ANDROID__
    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
    };
#else
    const int attribs[] = { 0 };
#endif

    EGLConfig config;
    int numConfigs;
    if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs == 0) {
        std::cerr << "eglChooseConfig failed." << std::endl;
        return false;
    }

#ifdef __ANDROID__
    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
#else
    const int contextAttribs[] = { 0 };
#endif

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        std::cerr << "eglCreateContext failed." << std::endl;
        return false;
    }

    m_eglDisplay = display;
    m_eglContext = context;
    m_eglConfig = config;

    return true;
}

void GLESRHI::shutdown() {
    std::cout << "Shutting down GLES RHI..." << std::endl;

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
            std::cerr << "eglCreateWindowSurface failed." << std::endl;
            return;
        }

        if (eglMakeCurrent(display, (EGLSurface)m_eglSurface, (EGLSurface)m_eglSurface, (EGLContext)m_eglContext) != EGL_TRUE) {
            std::cerr << "eglMakeCurrent failed." << std::endl;
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
