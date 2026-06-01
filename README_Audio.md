# 音频功能集成说明

在最新的提交中，我们在现有的高性能视频渲染管线上，成功补齐了**音频采集与编码**链路，使导出的 MP4 文件具备完整的音轨：

## 1. Oboe 音频采集 (`AudioCapture`)
* 引入了 Google 官方的 `Oboe` 高性能 C++ 音频库。
* 使用 `NdkAudioCapture` 开启 LowLatency 模式，以 48kHz、双声道 (Stereo)、16-bit PCM 格式实时捕获麦克风数据。
* Oboe 的回调 (`onAudioReady`) 运行在独立的高优先级音频线程，极大降低了采集延迟。

## 2. 统一媒体编码器 (`MediaEncoder`)
* 将原本的 `VideoEncoder` 升级为 `MediaEncoder`，同时管理视频轨 (H.265/H.264) 和音频轨。
* 音频使用 NDK 的 `AMediaCodec` 进行硬件/软件 AAC 编码 (`audio/mp4a-latm`)。
* 接收来自 Oboe 的 PCM 数据，将其喂入 AAC 编码器的 InputBuffer 中。

## 3. 音视频同步与混合 (A/V Sync & Muxing)
* **时间戳同步 (PTS)**：由于渲染线程 (Video) 和音频线程 (Audio) 独立运行，我们建立了一个全局录制基准时钟 `m_startTimeNs`。所有进入编码器的画面和 PCM 音频块，都会被换算成相对于该起始点的微秒级时间戳 (PTS)。
* `drainInternal()` 循环会同时拉取视频轨和音频轨的编码输出 (OutputBuffer)，确保 `AMediaMuxer` 收到的交织数据严格按照时间戳对齐，从而实现最终导出 `.mp4` 时的音画同步。
