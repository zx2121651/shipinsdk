#include "GLESRHI.h"
#include <iostream>
#include <vector>
#include <functional>

#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <android/native_window.h>
#include <android/log.h>

#define LOG_TAG "VFX_GLES_RHI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#else
// Stub definitions for non-Android builds to compile
typedef void* EGLDisplay;
typedef void* EGLContext;
typedef void* EGLSurface;
typedef void* EGLConfig;
#define EGL_NO_DISPLAY nullptr
#define EGL_NO_CONTEXT nullptr
#define EGL_NO_SURFACE nullptr
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
inline void glGenTextures(int, unsigned int*) {}
inline void glBindTexture(int, unsigned int) {}
inline void glTexParameteri(int, int, int) {}
inline void glTexImage2D(int, int, int, int, int, int, int, int, const void*) {}
inline void glDeleteTextures(int, const unsigned int*) {}
inline void glGenFramebuffers(int, unsigned int*) {}
inline void glBindFramebuffer(int, unsigned int) {}
inline void glFramebufferTexture2D(int, int, int, unsigned int, int) {}
inline void glDeleteFramebuffers(int, const unsigned int*) {}
inline void glViewport(int, int, int, int) {}
inline void glClearColor(float, float, float, float) {}
inline void glClear(unsigned int) {}
inline unsigned int glCreateShader(int) { return 0; }
inline void glShaderSource(unsigned int, int, const char**, const int*) {}
inline void glCompileShader(unsigned int) {}
inline void glGetShaderiv(unsigned int, int, int*) {}
inline void glGetShaderInfoLog(unsigned int, int, int*, char*) {}
inline void glDeleteShader(unsigned int) {}
inline unsigned int glCreateProgram() { return 0; }
inline void glAttachShader(unsigned int, unsigned int) {}
inline void glLinkProgram(unsigned int) {}
inline void glGetProgramiv(unsigned int, int, int*) {}
inline void glGetProgramInfoLog(unsigned int, int, int*, char*) {}
inline void glDeleteProgram(unsigned int) {}
inline void glUseProgram(unsigned int) {}
inline int glGetUniformLocation(unsigned int, const char*) { return -1; }
inline void glUniform1i(int, int) {}
inline void glUniformMatrix4fv(int, int, int, const float*) {}
inline void glActiveTexture(unsigned int) {}
inline void glEnableVertexAttribArray(unsigned int) {}
inline void glVertexAttribPointer(unsigned int, int, int, int, int, const void*) {}
inline void glDrawArrays(int, int, int) {}
inline void glGenBuffers(int, unsigned int*) {}
inline void glBindBuffer(int, unsigned int) {}
inline void glBufferData(int, int, const void*, int) {}
inline void glDeleteBuffers(int, const unsigned int*) {}
#define GL_FLOAT 0
#define GL_FALSE 0
#define GL_TRIANGLE_STRIP 0
#define GL_ARRAY_BUFFER 0
#define GL_STATIC_DRAW 0
#define GL_COLOR_BUFFER_BIT 0
#define GL_VERTEX_SHADER 0
#define GL_FRAGMENT_SHADER 0
#define GL_COMPILE_STATUS 0
#define GL_LINK_STATUS 0
#define GL_TEXTURE0 0
#define GL_RGBA 0
#define GL_UNSIGNED_BYTE 0
#define GL_TEXTURE_MIN_FILTER 0
#define GL_TEXTURE_MAG_FILTER 0
#define GL_LINEAR 0
#define GL_CLAMP_TO_EDGE 0
#define GL_TEXTURE_WRAP_S 0
#define GL_TEXTURE_WRAP_T 0
#define GL_FRAMEBUFFER 0
#define GL_COLOR_ATTACHMENT0 0
#define LOGI(...)
#define LOGE(...)
#endif

namespace vfx {

// Helper: Compile shader
static unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        LOGE("Shader compilation failed: %s", infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// ------------------------------------------------------------------
// PURE RHI IMPLEMENTATIONS FOR GLES
// ------------------------------------------------------------------

class GLESTexture : public ITexture {
public:
    GLESTexture(unsigned int id, int w, int h, TextureType t, bool owns)
        : textureId(id), width(w), height(h), type(t), ownsTexture(owns) {}

    ~GLESTexture() override {
        if (ownsTexture && textureId != 0) {
            glDeleteTextures(1, &textureId);
        }
    }

    void* getNativeHandle() const override {
        // Return ID cast to void* for legacy interop if needed
        return reinterpret_cast<void*>(static_cast<uintptr_t>(textureId));
    }

    TextureType getType() const override { return type; }
    int getWidth() const override { return width; }
    int getHeight() const override { return height; }

    unsigned int textureId;
    int width;
    int height;
    TextureType type;
    bool ownsTexture;
};

class GLESRenderTarget : public IRenderTarget {
public:
    GLESRenderTarget(unsigned int fbo, std::shared_ptr<ITexture> tex)
        : fboId(fbo), texture(tex) {}

    ~GLESRenderTarget() override {
        if (fboId != 0) {
            glDeleteFramebuffers(1, &fboId);
        }
    }

    std::shared_ptr<ITexture> getTexture() const override { return texture; }

    unsigned int fboId;
    std::shared_ptr<ITexture> texture;
};

class GLESShader : public IShader {
public:
    GLESShader(unsigned int pId) : programId(pId) {}
    ~GLESShader() override {
        if (programId != 0) glDeleteProgram(programId);
    }
    unsigned int programId;
};

class GLESPipelineState : public IPipelineState {
public:
    GLESPipelineState(std::shared_ptr<GLESShader> s) : shader(s) {}
    std::shared_ptr<GLESShader> shader;
};

class GLESCommandBuffer : public ICommandBuffer {
public:
    GLESCommandBuffer(GLESRHI* rhi) : m_rhi(rhi) {}

    ~GLESCommandBuffer() override = default;

    void begin() override {
        m_commands.clear();
    }

    void beginRenderPass(const RenderPassDescriptor& desc) override {
        m_commands.push_back([this, desc]() {
            // First, switch EGL Context if necessary
            m_rhi->makeContextCurrent(desc.isEncoderTarget);

            if (desc.colorAttachment) {
                auto glesRt = std::static_pointer_cast<GLESRenderTarget>(desc.colorAttachment);
                glBindFramebuffer(GL_FRAMEBUFFER, glesRt->fboId);
                auto tex = glesRt->getTexture();
                if (tex) glViewport(0, 0, tex->getWidth(), tex->getHeight());
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                // Viewport should ideally be passed in RenderPassDescriptor, assuming 1280x720 fallback for now
                glViewport(0, 0, 720, 1280);
            }

            if (desc.clearColor) {
                glClearColor(desc.clearColorValue[0], desc.clearColorValue[1], desc.clearColorValue[2], desc.clearColorValue[3]);
                glClear(GL_COLOR_BUFFER_BIT);
            }
        });
    }

    void bindPipelineState(std::shared_ptr<IPipelineState> pso) override {
        m_commands.push_back([this, pso]() {
            auto glesPso = std::static_pointer_cast<GLESPipelineState>(pso);
            m_currentProgram = glesPso->shader->programId;
            glUseProgram(m_currentProgram);
        });
    }

    void bindTexture(int slot, std::shared_ptr<ITexture> texture) override {
        m_commands.push_back([this, slot, texture]() {
            auto glesTex = std::static_pointer_cast<GLESTexture>(texture);
            glActiveTexture(GL_TEXTURE0 + slot);
            int target = (glesTex->type == TextureType::TextureExternal) ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;
            glBindTexture(target, glesTex->textureId);

            if (m_currentProgram != 0) {
                int loc = glGetUniformLocation(m_currentProgram, "uTexture");
                if (loc >= 0) glUniform1i(loc, slot);
            }
        });
    }

    void pushConstants(const void* data, size_t size) override {
        // Legacy polyfill: we map a 16-float array to whatever uniforms are needed by the shader.
        // OESCameraFilter needs "uTransformMatrix".
        // BeautyFilter needs "uTexelSize", "uSmoothing", "uWhitening".
        if (size == 16 * sizeof(float)) {
            std::vector<float> params((const float*)data, (const float*)data + 16);
            m_commands.push_back([this, params]() {
                if (m_currentProgram != 0) {
                    int locTransform = glGetUniformLocation(m_currentProgram, "uTransformMatrix");
                    if (locTransform >= 0) {
                        glUniformMatrix4fv(locTransform, 1, GL_FALSE, params.data());
                    }

                    int locTexelSize = glGetUniformLocation(m_currentProgram, "uTexelSize");
                    if (locTexelSize >= 0) {
                        // Assuming glUniform2f is available, or use an array. Here we fallback to NDK manual linkage if needed.
                        // We will map via raw pointer for GLES2 compatibility.
                        float texelSize[2] = { params[0], params[1] };
                        typedef void (*glUniform2fv_t)(int, int, const float*);
                        static auto glUniform2fv_ptr = (glUniform2fv_t)eglGetProcAddress("glUniform2fv");
                        if (glUniform2fv_ptr) glUniform2fv_ptr(locTexelSize, 1, texelSize);
                    }

                    int locSmoothing = glGetUniformLocation(m_currentProgram, "uSmoothing");
                    if (locSmoothing >= 0) {
                        typedef void (*glUniform1f_t)(int, float);
                        static auto glUniform1f_ptr = (glUniform1f_t)eglGetProcAddress("glUniform1f");
                        if (glUniform1f_ptr) glUniform1f_ptr(locSmoothing, params[2]);
                    }

                    int locWhitening = glGetUniformLocation(m_currentProgram, "uWhitening");
                    if (locWhitening >= 0) {
                        typedef void (*glUniform1f_t)(int, float);
                        static auto glUniform1f_ptr = (glUniform1f_t)eglGetProcAddress("glUniform1f");
                        if (glUniform1f_ptr) glUniform1f_ptr(locWhitening, params[3]);
                    }
                }
            });
        }
    }

    void drawFullScreenQuad() override {
        m_commands.push_back([this]() {
            glBindBuffer(GL_ARRAY_BUFFER, m_rhi->getQuadVBO());

            // Assume standard attribute locations for 2D filter shaders
            // aPosition = 0, aTexCoord = 1
            int posLoc = 0; // Better: glGetAttribLocation(m_currentProgram, "aPosition");
            int texLoc = 1; // Better: glGetAttribLocation(m_currentProgram, "aTexCoord");

            if (m_currentProgram != 0) {
                // Use standard GLES API pointer fetching if symbol is not directly linked
                typedef int (*glGetAttribLocation_t)(unsigned int, const char*);
                static auto glGetAttribLocation_ptr = (glGetAttribLocation_t)eglGetProcAddress("glGetAttribLocation");
                if (glGetAttribLocation_ptr) {
                    posLoc = glGetAttribLocation_ptr(m_currentProgram, "aPosition");
                    texLoc = glGetAttribLocation_ptr(m_currentProgram, "aTexCoord");
                }
            }

            if (posLoc >= 0) {
                glEnableVertexAttribArray(posLoc);
                glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            }
            if (texLoc >= 0) {
                glEnableVertexAttribArray(texLoc);
                glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            }

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        });
    }

    void endRenderPass() override {
        // GL has no explicit end pass, maybe resolve MSAA or invalidate attachments here
    }

    void end() override {
        // Mark end of recording
    }

    void submit() override {
        for (auto& cmd : m_commands) {
            cmd();
        }
    }

private:
    GLESRHI* m_rhi;
    unsigned int m_vbo = 0;
    unsigned int m_currentProgram = 0;
    std::vector<std::function<void()>> m_commands;

};

// ------------------------------------------------------------------
// GLESRHI
// ------------------------------------------------------------------

GLESRHI::GLESRHI()
    : m_eglDisplay(nullptr), m_eglContext(nullptr), m_eglConfig(nullptr),
      m_eglSurfaceMain(nullptr), m_eglSurfaceEncoder(nullptr) {}

GLESRHI::~GLESRHI() {
    shutdown();
}

bool GLESRHI::initialize(const HardwareCapabilities& caps) {
#ifdef __ANDROID__
    m_eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (m_eglDisplay == EGL_NO_DISPLAY) return false;

    EGLint major, minor;
    if (!eglInitialize((EGLDisplay)m_eglDisplay, &major, &minor)) return false;

    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_NONE
    };

    EGLint numConfigs;
    if (!eglChooseConfig((EGLDisplay)m_eglDisplay, attribs, (EGLConfig*)&m_eglConfig, 1, &numConfigs)) return false;

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    m_eglContext = eglCreateContext((EGLDisplay)m_eglDisplay, (EGLConfig)m_eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (m_eglContext == EGL_NO_CONTEXT) return false;

    // Create a dummy pbuffer surface so we have a current context even without a window
    EGLint pbufferAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    m_eglSurfaceMain = eglCreatePbufferSurface((EGLDisplay)m_eglDisplay, (EGLConfig)m_eglConfig, pbufferAttribs);
    eglMakeCurrent((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceMain, (EGLSurface)m_eglSurfaceMain, (EGLContext)m_eglContext);

    // Prepare global full screen quad VBO for GLES immediate mode translation
    static float vertices[] = {
        // x, y, u, v
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    LOGI("GLESRHI initialized successfully.");
    return true;
#else
    return true;
#endif
}

void GLESRHI::shutdown() {
#ifdef __ANDROID__
    if (m_eglDisplay != EGL_NO_DISPLAY) {
        if (m_vbo != 0) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        eglMakeCurrent((EGLDisplay)m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (m_eglContext != EGL_NO_CONTEXT) eglDestroyContext((EGLDisplay)m_eglDisplay, (EGLContext)m_eglContext);
        if (m_eglSurfaceMain != EGL_NO_SURFACE) eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceMain);
        if (m_eglSurfaceEncoder != EGL_NO_SURFACE) eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceEncoder);
        eglTerminate((EGLDisplay)m_eglDisplay);
    }
    m_eglDisplay = EGL_NO_DISPLAY;
    m_eglContext = EGL_NO_CONTEXT;
    m_eglSurfaceMain = EGL_NO_SURFACE;
    m_eglSurfaceEncoder = EGL_NO_SURFACE;
#endif
}

void GLESRHI::setWindow(void* window) {
#ifdef __ANDROID__
    if (m_eglDisplay == EGL_NO_DISPLAY) return;
    if (m_eglSurfaceMain != EGL_NO_SURFACE) {
        eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceMain);
        m_eglSurfaceMain = EGL_NO_SURFACE;
    }
    if (window) {
        m_eglSurfaceMain = eglCreateWindowSurface((EGLDisplay)m_eglDisplay, (EGLConfig)m_eglConfig, (EGLNativeWindowType)window, nullptr);
    } else {
        EGLint pbufferAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
        m_eglSurfaceMain = eglCreatePbufferSurface((EGLDisplay)m_eglDisplay, (EGLConfig)m_eglConfig, pbufferAttribs);
    }
#endif
}

void GLESRHI::makeMainWindowCurrent() {
    makeContextCurrent(false);
}

void GLESRHI::makeEncoderWindowCurrent() {
    makeContextCurrent(true);
}

void GLESRHI::setEncoderWindow(void* window) {
#ifdef __ANDROID__
    if (m_eglDisplay == EGL_NO_DISPLAY) return;
    if (m_eglSurfaceEncoder != EGL_NO_SURFACE) {
        eglDestroySurface((EGLDisplay)m_eglDisplay, (EGLSurface)m_eglSurfaceEncoder);
        m_eglSurfaceEncoder = EGL_NO_SURFACE;
    }
    if (window) {
        m_eglSurfaceEncoder = eglCreateWindowSurface((EGLDisplay)m_eglDisplay, (EGLConfig)m_eglConfig, (EGLNativeWindowType)window, nullptr);
    }
#endif
}

void GLESRHI::makeContextCurrent(bool encoderSurface) {
#ifdef __ANDROID__
    if (m_eglDisplay != EGL_NO_DISPLAY && m_eglContext != EGL_NO_CONTEXT) {
        EGLSurface surface = encoderSurface ? (EGLSurface)m_eglSurfaceEncoder : (EGLSurface)m_eglSurfaceMain;
        if (surface != EGL_NO_SURFACE) {
            eglMakeCurrent((EGLDisplay)m_eglDisplay, surface, surface, (EGLContext)m_eglContext);
        }
    }
#endif
}

void GLESRHI::present(bool encoderSurface) {
#ifdef __ANDROID__
    if (m_eglDisplay != EGL_NO_DISPLAY) {
        EGLSurface surface = encoderSurface ? (EGLSurface)m_eglSurfaceEncoder : (EGLSurface)m_eglSurfaceMain;
        if (surface != EGL_NO_SURFACE) {
            eglSwapBuffers((EGLDisplay)m_eglDisplay, surface);
        }
    }
#endif
}

std::shared_ptr<IShader> GLESRHI::createShader(const std::string& vertexSource, const std::string& fragmentSource) {
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSource.c_str());
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str());
    if (vs == 0 || fs == 0) return nullptr;

    unsigned int programId = glCreateProgram();
    glAttachShader(programId, vs);
    glAttachShader(programId, fs);
    glLinkProgram(programId);

    glDeleteShader(vs);
    glDeleteShader(fs);

    int linked;
    glGetProgramiv(programId, GL_LINK_STATUS, &linked);
    if (!linked) {
        char infoLog[512];
        glGetProgramInfoLog(programId, 512, nullptr, infoLog);
        LOGE("Program linking failed: %s", infoLog);
        glDeleteProgram(programId);
        return nullptr;
    }

    return std::make_shared<GLESShader>(programId);
}

std::shared_ptr<IPipelineState> GLESRHI::createPipelineState(std::shared_ptr<IShader> shader) {
    auto glesShader = std::static_pointer_cast<GLESShader>(shader);
    return std::make_shared<GLESPipelineState>(glesShader);
}

std::shared_ptr<ITexture> GLESRHI::createTexture(int width, int height, TextureType type) {
    unsigned int textureId;
    glGenTextures(1, &textureId);
    int target = (type == TextureType::TextureExternal) ? GL_TEXTURE_EXTERNAL_OES : GL_TEXTURE_2D;

    glBindTexture(target, textureId);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (type == TextureType::Texture2D) {
        glTexImage2D(target, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }
    glBindTexture(target, 0);

    return std::make_shared<GLESTexture>(textureId, width, height, type, true);
}

std::shared_ptr<ITexture> GLESRHI::createTextureFromNative(void* nativeHandle, int width, int height, TextureType type) {
    unsigned int textureId = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(nativeHandle));
    // ownsTexture = false, we do not delete this texture ID as it was generated outside or we only wrap it
    return std::make_shared<GLESTexture>(textureId, width, height, type, false);
}

std::shared_ptr<IRenderTarget> GLESRHI::createRenderTarget(int width, int height) {
    auto texture = createTexture(width, height, TextureType::Texture2D);
    auto glesTex = std::static_pointer_cast<GLESTexture>(texture);

    unsigned int fboId;
    glGenFramebuffers(1, &fboId);
    glBindFramebuffer(GL_FRAMEBUFFER, fboId);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glesTex->textureId, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::make_shared<GLESRenderTarget>(fboId, texture);
}

std::shared_ptr<ICommandBuffer> GLESRHI::createCommandBuffer() {
    return std::make_shared<GLESCommandBuffer>(this);
}

} // namespace vfx
