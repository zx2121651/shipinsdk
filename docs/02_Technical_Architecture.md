# VFX Engine SDK 技术架构文档 (Technical Architecture)

## 1. 核心设计原则
系统采用**标准 C++20** 开发，严格遵循**跨平台**和**高性能**原则：
* 禁止将重量级计算（如大模型推理、图形管线编译、编解码协商）写在 UI 主线程或业务浅层。
* 摒弃过时且臃肿的第三方库，视频与音频处理直接采用系统级 NDK 硬件加速接口（`AMediaCodec`, `Oboe` 等）。

## 2. 线程模型 (Threading Model)
系统采用极度解耦的多线程架构来保障界面的绝对流畅：
* **UI 线程 (主线程)**：仅负责处理触摸事件和触发系统 API（如 CameraX 初始化），不执行任何图形相关指令。
* **RenderThread (渲染线程)**：SDK 内部持有一个常驻线程，并配备 `TaskQueue`。所有的 OpenGL ES 上下文创建、纹理生成、特效图执行、以及编码器的起停操作，均通过闭包 (Lambda) 被异步 `postTask` 到此线程执行。
* **AudioThread (音频采集线程)**：由 Oboe 内部接管的高优先级实时线程，专门负责在极短的回调周期内搬运麦克风 PCM 数据。

## 3. 图形管线架构

### 3.1 显式图形硬件接口 (Explicit RHI)
系统设计了 `IRHI` 层以隔离具体的图形 API。
* **按需初始化**：通过 `HardwareCapabilities` 结构，自动探测设备的 OpenGL ES 版本 (3.2 向下兼容到 2.0)，并在未来平滑扩展至 Vulkan 和 Metal。
* **多目标渲染 (Multi-Surface)**：RHI 支持同时绑定两个 `ANativeWindow` 句柄（一个是手机屏幕，一个是硬件编码器的入口）。`RenderGraph` 计算完滤镜后，分别在两个 EGLSurface 上调用 `makeCurrent` 和 `swapBuffers`，实现一次渲染、两处输出。

### 3.2 零拷贝与外部纹理 (Zero-Copy & OES)
在 Android 上，摄像头数据通过 `SurfaceTexture` 传递至 C++。引擎利用 `GL_TEXTURE_EXTERNAL_OES` 将相机的 YUV 硬件 Buffer 直接映射为 GPU 内部纹理，避免了任何 CPU 级别的像素级干预（如 `glReadPixels`），将数据传输延迟降至理论极小值。

## 4. 音视频同步与硬件编码架构

### 4.1 独立轨道编码
* **Video**：利用 `AMediaCodec` 构建 `video/hevc` 或 `video/avc` 硬件编码器，由 RHI 的 EGL 上下文直接把画面注入编码器输入表面。
* **Audio**：利用 `AMediaCodec` 构建 `audio/mp4a-latm` (AAC) 硬件或软件编码器，由 Oboe 回调直接填充 PCM 缓冲区。

### 4.2 全局 PTS 相对时间同步机制
安卓硬件视频编码器输出的 `info.presentationTimeUs` 默认是基于设备开机时长 (Uptime) 的绝对纳秒数。若直接混流，会导致播放器时间轴崩溃。
本系统采用了**录像基准时间修正法**：
1. 录制开始瞬间，记录一个全局的时间戳 `m_startTimeNs` (基于 `steady_clock`)。
2. 音频数据进入时：音频包 PTS = `(当前绝对时间 - m_startTimeNs) / 1000`。
3. 视频数据抽出时：视频包 PTS = `原始硬件绝对时间 - (m_startTimeNs / 1000)`。
4. Muxer (`AMediaMuxer`) 收到的所有数据包均变为从 `0` 开始单调递增的相对时间戳，确保了严丝合缝的 A/V 同步。
