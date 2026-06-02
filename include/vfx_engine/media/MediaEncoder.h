#pragma once

#include <string>
#include <memory>
#include <cstdint>
#include "vfx_engine/media/VideoEncoder.h" // For VideoCodecType

namespace vfx {

/**
 * @brief 统一媒体编码器接口
 * 该类封装了底层的硬件编码器（如 Android NDK AMediaCodec）和混流器（AMediaMuxer）。
 * 支持双轨并发：将 GPU 渲染的画面直接编码为 H.265/H.264，并将 PCM 数据编码为 AAC 音频，
 * 最后将两路码流通过严格的 PTS 时间戳同步封装至 MP4 文件中。
 */
class MediaEncoder {
public:
    virtual ~MediaEncoder() = default;

    /**
     * @brief 初始化并启动编码器与混流器
     * @param outputPath MP4 视频导出的绝对路径
     * @param width 视频编码宽度
     * @param height 视频编码高度
     * @param requestedCodec 请求的视频编码格式（如果硬件不支持 H.265，会自动降级为 H.264）
     * @param enableAudio 是否开启音频编码轨（AAC）
     * @return 启动成功返回 true，失败返回 false
     */
    virtual bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec = VideoCodecType::H264, bool enableAudio = true) = 0;

    /**
     * @brief 停止录制
     * 该方法会发送 EOS (End of Stream) 信号，抽干(Drain)编码器中最后残留的缓冲帧，并安全关闭混流器，完成文件写入。
     */
    virtual void stop() = 0;

    /**
     * @brief 抽取编码数据并写入文件
     * 定期从音视频硬件编码器中拉取(Dequeue)压缩好的数据包，并将其交织写入混流器。
     */
    virtual void drain() = 0;

    /**
     * @brief 提交 PCM 原始音频数据给 AAC 编码器
     * @param audioData 16-bit PCM 音频缓冲区
     * @param numFrames 音频帧数（双声道则每个帧含左右两个采样）
     * @param timestampNs 音频帧的绝对系统时间戳（纳秒）
     */
    virtual void encodeAudioFrame(const int16_t* audioData, int numFrames, int64_t timestampNs) = 0;

    /**
     * @brief 获取视频编码器的原生输入表面
     * 用于跨平台 RHI (Render Hardware Interface) 作为独立的 EGLSurface 渲染目标，
     * 从而实现 GPU 直接把结果画入编码器 (零 CPU 拷贝)。
     * @return 原生窗口指针（如 Android 上的 ANativeWindow*）
     */
    virtual void* getInputWindow() = 0;

    /**
     * @brief 通知编码器有一帧视频画面已通过 GPU 绘制到 InputWindow 上
     * 引擎在 swapBuffers 后会调用此方法，触发编码器消费新画面并抽干数据。
     */
    virtual void notifyFrameReady() = 0;

    /**
     * @brief 工厂方法，根据当前平台自动创建对应的编码器实现
     */
    static std::shared_ptr<MediaEncoder> create();
};

} // namespace vfx
