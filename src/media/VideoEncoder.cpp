#include "vfx_engine/media/VideoEncoder.h"
#include <iostream>

#ifdef __ANDROID__
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaMuxer.h>
#include <media/NdkMediaFormat.h>
#include <android/log.h>
#include <android/native_window.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>

#define LOG_TAG "VFX_ENCODER"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace vfx {

class NdkVideoEncoder : public VideoEncoder {
public:
    NdkVideoEncoder() {}
    ~NdkVideoEncoder() { stop(); }

    bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec) override {
        LOGI("Starting VideoEncoder (NDK), path: %s", outputPath.c_str());
        m_width = width;
        m_height = height;

        int fd = open(outputPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd < 0) {
            LOGE("Failed to open output file");
            return false;
        }

        m_muxer = AMediaMuxer_new(fd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
        close(fd);
        if (!m_muxer) return false;

        const char* mimeType = (requestedCodec == VideoCodecType::H265) ? "video/hevc" : "video/avc";

        m_codec = AMediaCodec_createEncoderByType(mimeType);
        if (!m_codec && requestedCodec == VideoCodecType::H265) {
            LOGI("H.265 (HEVC) encoder creation failed. Falling back to H.264 (AVC).");
            mimeType = "video/avc";
            m_codec = AMediaCodec_createEncoderByType(mimeType);
        }

        if (!m_codec) {
            LOGE("Failed to create video codec for %s", mimeType);
            return false;
        }

        LOGI("Successfully created codec for %s", mimeType);

        AMediaFormat* format = AMediaFormat_new();
        AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mimeType);
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, width);
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, height);
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, 2000000);
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, 30);
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1);
        AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, 2130708361); // COLOR_FormatSurface

        if (AMediaCodec_configure(m_codec, format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE) != AMEDIA_OK) {
            LOGE("Failed to configure codec");
            AMediaFormat_delete(format);
            return false;
        }
        AMediaFormat_delete(format);

        if (AMediaCodec_createInputSurface(m_codec, &m_window) != AMEDIA_OK) return false;
        if (AMediaCodec_start(m_codec) != AMEDIA_OK) return false;

        m_isRecording = true;
        m_startTime = std::chrono::steady_clock::now();
        LOGI("VideoEncoder successfully started.");
        return true;
    }

    void stop() override {
        if (!m_isRecording) return;
        m_isRecording = false;

        if (m_codec) {
            AMediaCodec_signalEndOfInputStream(m_codec);
            drain();
            AMediaCodec_stop(m_codec);
            AMediaCodec_delete(m_codec);
            m_codec = nullptr;
        }

        if (m_muxer) {
            if (m_muxerStarted) {
                AMediaMuxer_stop(m_muxer);
                m_muxerStarted = false;
            }
            AMediaMuxer_delete(m_muxer);
            m_muxer = nullptr;
        }

        if (m_window) {
            ANativeWindow_release(m_window);
            m_window = nullptr;
        }
    }

    void drain() override {
        if (!m_codec || !m_muxer) return;

        while (true) {
            AMediaCodecBufferInfo info;
            ssize_t status = AMediaCodec_dequeueOutputBuffer(m_codec, &info, 0);

            if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                break;
            } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                if (!m_muxerStarted) {
                    AMediaFormat* newFormat = AMediaCodec_getOutputFormat(m_codec);
                    m_trackIndex = AMediaMuxer_addTrack(m_muxer, newFormat);
                    AMediaFormat_delete(newFormat);
                    AMediaMuxer_start(m_muxer);
                    m_muxerStarted = true;
                }
            } else if (status >= 0) {
                size_t bufSize = 0;
                uint8_t* buf = AMediaCodec_getOutputBuffer(m_codec, status, &bufSize);

                if (buf && m_muxerStarted && info.size != 0) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - m_startTime).count();
                    info.presentationTimeUs = elapsed;
                    AMediaMuxer_writeSampleData(m_muxer, m_trackIndex, buf, &info);
                }

                AMediaCodec_releaseOutputBuffer(m_codec, status, false);
                if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) break;
            } else {
                break;
            }
        }
    }

    void* getInputWindow() override { return m_window; }
    void notifyFrameReady() override { drain(); }

private:
    bool m_isRecording = false;
    bool m_muxerStarted = false;
    int m_width = 0, m_height = 0;
    ssize_t m_trackIndex = -1;

    AMediaCodec* m_codec = nullptr;
    AMediaMuxer* m_muxer = nullptr;
    ANativeWindow* m_window = nullptr;
    std::chrono::steady_clock::time_point m_startTime;
};

std::shared_ptr<VideoEncoder> VideoEncoder::create() {
    return std::make_shared<NdkVideoEncoder>();
}

} // namespace vfx
#else
namespace vfx {
class MockVideoEncoder : public VideoEncoder {
public:
    bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec) override { return true; }
    void stop() override {}
    void drain() override {}
    void* getInputWindow() override { return nullptr; }
    void notifyFrameReady() override {}
};
std::shared_ptr<VideoEncoder> VideoEncoder::create() {
    return std::make_shared<MockVideoEncoder>();
}
}
#endif
