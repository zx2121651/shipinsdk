# VFX Engine SDK 接口文档 (API Reference)

本章节列出对外暴露的核心 C++ 接口和提供给上层业务环境的 JNI 桥接接口。

## 1. 核心 C++ API

### 1.1 `vfx::MediaEncoder` (媒体编码器)
位于 `include/vfx_engine/media/MediaEncoder.h`

```cpp
// 初始化编码器
// outputPath: 导出的 mp4 路径
// width, height: 导出分辨率
// requestedCodec: 请求的编码格式 (H265 / H264)
// enableAudio: 是否开启 AAC 录音
virtual bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec, bool enableAudio) = 0;

// 停止录制并生成 MP4 文件
virtual void stop() = 0;

// 将 PCM 音频压入编码器 (内部处理 PTS 同步)
virtual void encodeAudioFrame(const int16_t* audioData, int numFrames, int64_t timestampNs) = 0;

// 获取视频硬件编码器的输入平面句柄 (供 RHI 渲染)
virtual void* getInputWindow() = 0;
```

### 1.2 `vfx::AudioCapture` (音频采集器)
位于 `include/vfx_engine/media/AudioCapture.h`

```cpp
// 绑定音频回调函数，接收底层 PCM 数据和时间戳
virtual void setCallback(AudioDataCallback callback) = 0;

// 启动麦克风
virtual bool start(int sampleRate, int channelCount) = 0;

// 停止麦克风
virtual void stop() = 0;
```

### 1.3 `vfx::IRHI` (显式图形接口)
位于 `include/vfx_engine/rhi/RHI.h`
通过 `vfx::createRHI(BackendType, caps)` 实例化。
负责管理 EGL 环境、纹理生命周期、窗口绑定以及上下文切换。

## 2. JNI 接口 (供 Android UI 调用)
定义在 `com.vfx.core.VfxEngine` 中。

```java
// 初始化引擎，传递设备的 GLES 版本和硬件能力。此方法会启动 C++ RenderThread。
public native void init(int glesVersionHex, boolean isVulkanSupported);

// 销毁引擎，安全地关闭所有资源、终止异步线程和 JNI 引用。
public native void destroy();

// 将 Android 的 UI Surface (如 TextureView/SurfaceView) 绑定到 C++ 引擎，作为预览的目标。
public native void setSurface(Surface surface);

// 请求 C++ 在渲染线程生成一个 OES 外部纹理。
// 生成完毕后，C++ 会反向调用 Java 层的 onCameraTextureGenerated 回调。
public native void generateCameraTexture();

// 设置摄像头输出的物理分辨率尺寸，用于计算滤镜矩阵。
public native void setCameraTextureSize(int width, int height);

// 核心渲染循环推进器。当 CameraX 捕获到新帧时调用此方法。
// 触发 C++ 层的 RenderGraph 进行特效计算并自动交换缓冲区 (SwapBuffers) 实现上屏或录像。
public native void notifyCameraFrameAvailable();

// 开始录像。
// outputPath: 文件保存路径 (如 /sdcard/cache/xxx.mp4)
// codecType: 1=H.265, 0=H.264
public native void startRecording(String outputPath, int codecType);

// 停止录像，生成最终文件。
public native void stopRecording();
```
