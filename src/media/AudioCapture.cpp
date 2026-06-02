#include "vfx_engine/media/AudioCapture.h"
#include <iostream>
#include <chrono>

#ifdef __ANDROID__
#include <oboe/Oboe.h>
#include <android/log.h>

#define LOG_TAG "VFX_AUDIO_CAPTURE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace vfx {

class NdkAudioCapture : public AudioCapture, public oboe::AudioStreamDataCallback {
public:
    NdkAudioCapture() = default;
    ~NdkAudioCapture() { stop(); }

    bool start(int sampleRate, int channelCount) override {
        // 初始化 Oboe 构造器
        // 显式指定：输入方向(录音)、低延迟模式、16-bit PCM 格式。
        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Input)
               ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
               ->setFormat(oboe::AudioFormat::I16)
               ->setSampleRate(sampleRate)
               ->setChannelCount(channelCount)
               ->setDataCallback(this); // 绑定当前对象作为数据回调接收者

        oboe::Result result = builder.openStream(m_stream);
        if (result != oboe::Result::OK) {
            LOGE("Failed to open audio stream: %s", oboe::convertToText(result));
            return false;
        }

        result = m_stream->requestStart();
        if (result != oboe::Result::OK) {
            LOGE("Failed to start audio stream: %s", oboe::convertToText(result));
            m_stream->close();
            return false;
        }

        LOGI("AudioCapture started successfully. Sample rate: %d, Channels: %d", sampleRate, channelCount);
        return true;
    }

    void stop() override {
        if (m_stream) {
            m_stream->requestStop();
            m_stream->close();
            m_stream.reset();
            LOGI("AudioCapture stopped.");
        }
    }

    void setCallback(AudioDataCallback callback) override {
        m_callback = callback;
    }

    // Oboe 底层高优先级音频线程回调函数
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numFrames) override {
        if (m_callback && audioData != nullptr && numFrames > 0) {
            // 计算当前的绝对时间戳（纳秒级别，基于 steady_clock）
            // 此时间戳将传递给 MediaEncoder，并在那里被转换为从 0 开始的相对 PTS 时间
            auto now = std::chrono::steady_clock::now();
            int64_t timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

            m_callback(static_cast<const int16_t*>(audioData), numFrames, timestampNs);
        }
        return oboe::DataCallbackResult::Continue; // 告诉底层持续录音
    }

private:
    std::shared_ptr<oboe::AudioStream> m_stream;
    AudioDataCallback m_callback;
};

std::shared_ptr<AudioCapture> AudioCapture::create() {
    return std::make_shared<NdkAudioCapture>();
}

} // namespace vfx
#else
namespace vfx {
class MockAudioCapture : public AudioCapture {
public:
    bool start(int sampleRate, int channelCount) override { return true; }
    void stop() override {}
    void setCallback(AudioDataCallback callback) override {}
};
std::shared_ptr<AudioCapture> AudioCapture::create() {
    return std::make_shared<MockAudioCapture>();
}
}
#endif
