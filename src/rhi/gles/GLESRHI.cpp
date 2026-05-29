#include "GLESRHI.h"
#include <iostream>

#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#define LOG_TAG "VFX_GLES"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) do {} while(0)
#define LOGE(...) do {} while(0)
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
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_OPENGL_ES3_BIT 0x0040

bool eglInitialize(EGLDisplay dpy, int* major, int* minor) { return true; }
EGLDisplay eglGetDisplay(void* display_id) { return (EGLDisplay)1; }
bool eglChooseConfig(EGLDisplay dpy, const int* attrib_list, EGLConfig* configs, int config_size, int* num_config) {
    if (num_config) *num_config = 1; return true;
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
unsigned int glCreateShader(unsigned int type) { return 1; }
void glShaderSource(unsigned int shader, int count, const char** string, const int* length) {}
void glCompileShader(unsigned int shader) {}
void glGetShaderiv(unsigned int shader, unsigned int pname, int* params) { *params = 1; }
void glGetShaderInfoLog(unsigned int shader, int bufSize, int* length, char* infoLog) {}
void glDeleteShader(unsigned int shader) {}

unsigned int glCreateProgram() { return 1; }
void glAttachShader(unsigned int program, unsigned int shader) {}
void glLinkProgram(unsigned int program) {}
void glGetProgramiv(unsigned int program, unsigned int pname, int* params) { *params = 1; }
void glUseProgram(unsigned int program) {}
int glGetUniformLocation(unsigned int program, const char* name) { return 0; }
int glGetAttribLocation(unsigned int program, const char* name) { return 0; }
void glDeleteProgram(unsigned int program) {}

void glGenBuffers(int n, unsigned int* buffers) {}
void glBindBuffer(unsigned int target, unsigned int buffer) {}
void glBufferData(unsigned int target, long size, const void* data, unsigned int usage) {}
void glEnableVertexAttribArray(unsigned int index) {}
void glVertexAttribPointer(unsigned int index, int size, unsigned int type, unsigned char normalized, int stride, const void* pointer) {}
void glDisableVertexAttribArray(unsigned int index) {}

void glActiveTexture(unsigned int texture) {}
void glBindTexture(unsigned int target, unsigned int texture) {}
void glUniformMatrix4fv(int location, int count, unsigned char transpose, const float* value) {}
void glUniform1i(int location, int v0) {}
void glDrawArrays(unsigned int mode, int first, int count) {}

#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FLOAT 0x1406
#define GL_FALSE 0
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#define GL_TRIANGLE_STRIP 0x0005
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

static const char* OES_VERTEX_SHADER = R"(
    attribute vec4 aPosition;
    attribute vec4 aTexCoord;
    uniform mat4 uTransformMatrix;
    varying vec2 vTexCoord;
    void main() {
        gl_Position = aPosition;
        vTexCoord = (uTransformMatrix * aTexCoord).xy;
    }
)";

static const char* OES_FRAGMENT_SHADER = R"(
    #extension GL_OES_EGL_image_external : require
    precision mediump float;
    varying vec2 vTexCoord;
    uniform samplerExternalOES uTexture;
    void main() {
        gl_FragColor = texture2D(uTexture, vTexCoord);
    }
)";

static unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLESRHI::GLESRHI()
    : m_eglDisplay(nullptr),
      m_eglContext(nullptr),
      m_eglConfig(nullptr),
      m_eglSurfaceMain(nullptr),
      m_eglSurfaceEncoder(nullptr) {
}

GLESRHI::~GLESRHI() {
    shutdown();
}

bool GLESRHI::setupOESPipeline() {
    if (m_oesProgram != 0) return true;

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, OES_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, OES_FRAGMENT_SHADER);

    if (!vertexShader || !fragmentShader) return false;

    m_oesProgram = glCreateProgram();
    glAttachShader(m_oesProgram, vertexShader);
    glAttachShader(m_oesProgram, fragmentShader);
    glLinkProgram(m_oesProgram);

    int success;
    glGetProgramiv(m_oesProgram, GL_LINK_STATUS, &success);
    if (!success) return false;

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    m_positionLocation = glGetAttribLocation(m_oesProgram, "aPosition");
    m_texCoordLocation = glGetAttribLocation(m_oesProgram, "aTexCoord");
    m_transformMatrixLocation = glGetUniformLocation(m_oesProgram, "uTransformMatrix");
    m_textureLocation = glGetUniformLocation(m_oesProgram, "uTexture");

    float vertices[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    return true;
}

bool GLESRHI::initialize() {
    LOGI("Initializing GLES RHI (EGL Context)...");

    EGLDisplay display = eglGetDisplay(nullptr);
    if (display == EGL_NO_DISPLAY) return false;
    if (eglInitialize(display, nullptr, nullptr) != EGL_TRUE) return false;

    int attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
        EGL_NONE
    };

    EGLConfig config;
    int numConfigs;
    bool isGLES3 = true;

    if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs == 0) {
        attribs[1] = EGL_OPENGL_ES2_BIT;
        isGLES3 = false;
        if (eglChooseConfig(display, attribs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs == 0) {
            return false;
        }
    }

    int contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, isGLES3 ? 3 : 2,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) return false;

    m_eglDisplay = display;
    m_eglContext = context;
    m_eglConfig = config;

    return true;
}

void GLESRHI::shutdown() {
    EGLDisplay display = (EGLDisplay)m_eglDisplay;
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_eglSurfaceMain != EGL_NO_SURFACE) eglDestroySurface(display, (EGLSurface)m_eglSurfaceMain);
        if (m_eglSurfaceEncoder != EGL_NO_SURFACE) eglDestroySurface(display, (EGLSurface)m_eglSurfaceEncoder);
        if (m_eglContext != EGL_NO_CONTEXT) eglDestroyContext(display, (EGLContext)m_eglContext);
        eglTerminate(display);
    }
}

void GLESRHI::setWindow(void* window) {
    EGLDisplay display = (EGLDisplay)m_eglDisplay;
    if (m_eglSurfaceMain != EGL_NO_SURFACE) {
        eglDestroySurface(display, (EGLSurface)m_eglSurfaceMain);
        m_eglSurfaceMain = nullptr;
    }

    if (window != nullptr) {
#ifdef __ANDROID__
        EGLNativeWindowType nativeWindow = static_cast<EGLNativeWindowType>(window);
#else
        void* nativeWindow = window;
#endif
        m_eglSurfaceMain = eglCreateWindowSurface(display, (EGLConfig)m_eglConfig, nativeWindow, nullptr);
        makeMainWindowCurrent();
        setupOESPipeline();
    } else {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}

void GLESRHI::setEncoderWindow(void* window) {
    EGLDisplay display = (EGLDisplay)m_eglDisplay;
    if (m_eglSurfaceEncoder != EGL_NO_SURFACE) {
        eglDestroySurface(display, (EGLSurface)m_eglSurfaceEncoder);
        m_eglSurfaceEncoder = nullptr;
    }

    if (window != nullptr) {
#ifdef __ANDROID__
        EGLNativeWindowType nativeWindow = static_cast<EGLNativeWindowType>(window);
#else
        void* nativeWindow = window;
#endif
        m_eglSurfaceEncoder = eglCreateWindowSurface(display, (EGLConfig)m_eglConfig, nativeWindow, nullptr);
    }
}

void GLESRHI::makeMainWindowCurrent() {
    if (m_eglDisplay && m_eglSurfaceMain) {
        eglMakeCurrent((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceMain, (EGLSurface)m_eglSurfaceMain, (EGLContext)m_eglContext);
    }
}

void GLESRHI::makeEncoderWindowCurrent() {
    if (m_eglDisplay && m_eglSurfaceEncoder) {
        eglMakeCurrent((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceEncoder, (EGLSurface)m_eglSurfaceEncoder, (EGLContext)m_eglContext);
    }
}

void GLESRHI::swapBuffers() {
    if (m_eglDisplay != EGL_NO_DISPLAY && m_eglSurfaceMain != EGL_NO_SURFACE) {
        eglSwapBuffers((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceMain);
    }
}

void GLESRHI::swapEncoderBuffers() {
    if (m_eglDisplay != EGL_NO_DISPLAY && m_eglSurfaceEncoder != EGL_NO_SURFACE) {
        eglSwapBuffers((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceEncoder);
    }
}

void GLESRHI::renderCameraOESTexture(int textureId, const float* transformMatrix) {
    if (!m_oesProgram) return;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_oesProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId);
    glUniform1i(m_textureLocation, 0);

    glUniformMatrix4fv(m_transformMatrixLocation, 1, GL_FALSE, transformMatrix);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glEnableVertexAttribArray(m_positionLocation);
    glVertexAttribPointer(m_positionLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(m_texCoordLocation);
    glVertexAttribPointer(m_texCoordLocation, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(m_positionLocation);
    glDisableVertexAttribArray(m_texCoordLocation);

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    glUseProgram(0);
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
