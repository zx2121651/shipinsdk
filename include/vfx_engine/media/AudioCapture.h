#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <cstdint>

namespace vfx {

/**
 * @brief 音频采集模块的跨平台抽象接口
 * 该模块负责从设备麦克风捕获底层音频数据，并通过回调函数将 PCM 数据传递给业务层或编码器。
 * 建议在独立的低延迟音频线程中运行。
 */
class AudioCapture {
public:
    /**
     * @brief 音频数据回调类型
     * @param audioData 指向 16-bit PCM 数据的指针
     * @param numFrames 本次采集到的音频帧数（注意：如果是双声道，数据总量为 numFrames * 2）
     * @param timestampNs 采集到该批次数据的相对时间戳（纳秒，通常基于系统启动时间）
     */
    using AudioDataCallback = std::function<void(const int16_t* audioData, int numFrames, int64_t timestampNs)>;

    virtual ~AudioCapture() = default;

    /**
     * @brief 启动音频采集流
     * @param sampleRate 采样率 (例如 48000 或 44100)
     * @param channelCount 声道数 (1 为单声道, 2 为双声道立体声)
     * @return 成功返回 true，失败返回 false
     */
    virtual bool start(int sampleRate, int channelCount) = 0;

    /**
     * @brief 停止音频采集并释放底层设备资源
     */
    virtual void stop() = 0;

    /**
     * @brief 设置接收 PCM 数据的回调函数
     * @param callback 用户提供的回调实现
     */
    virtual void setCallback(AudioDataCallback callback) = 0;

    /**
     * @brief 工厂方法，根据当前平台自动创建对应的音频采集实现 (如 Android 上的 Oboe)
     */
    static std::shared_ptr<AudioCapture> create();
};

} // namespace vfx
