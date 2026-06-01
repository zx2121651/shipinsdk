#include "vfx_engine/media/MediaEncoder.h"
#include <iostream>
#include <mutex>

#ifdef __ANDROID__
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaMuxer.h>
#include <media/NdkMediaFormat.h>
#include <android/log.h>
#include <android/native_window.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>

#define LOG_TAG "VFX_MEDIA_ENCODER"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace vfx {

class NdkMediaEncoder : public MediaEncoder {
public:
    NdkMediaEncoder() {}
    ~NdkMediaEncoder() { stop(); }

    bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec, bool enableAudio) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        LOGI("Starting MediaEncoder, path: %s", outputPath.c_str());

        int fd = open(outputPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd < 0) {
            LOGE("Failed to open output file");
            return false;
        }

        m_muxer = AMediaMuxer_new(fd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
        close(fd);
        if (!m_muxer) return false;

        // --- Video Codec Setup ---
        const char* videoMime = (requestedCodec == VideoCodecType::H265) ? "video/hevc" : "video/avc";
        m_videoCodec = AMediaCodec_createEncoderByType(videoMime);
        if (!m_videoCodec && requestedCodec == VideoCodecType::H265) {
            videoMime = "video/avc";
            m_videoCodec = AMediaCodec_createEncoderByType(videoMime);
        }

        if (m_videoCodec) {
            AMediaFormat* format = AMediaFormat_new();
            AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, videoMime);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, width);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, height);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, 2000000);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, 30);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1);
            AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, 2130708361); // COLOR_FormatSurface

            if (AMediaCodec_configure(m_videoCodec, format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE) != AMEDIA_OK) {
                LOGE("Failed to configure video codec");
                return false;
            }
            AMediaFormat_delete(format);
            AMediaCodec_createInputSurface(m_videoCodec, &m_window);
            AMediaCodec_start(m_videoCodec);
            m_hasVideo = true;
        }

        // --- Audio Codec Setup ---
        if (enableAudio) {
            m_audioCodec = AMediaCodec_createEncoderByType("audio/mp4a-latm");
            if (m_audioCodec) {
                AMediaFormat* format = AMediaFormat_new();
                AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, "audio/mp4a-latm");
                AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, 48000);
                AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, 2);
                AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, 128000);
                // AAC LC
                AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_AAC_PROFILE, 2);

                if (AMediaCodec_configure(m_audioCodec, format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE) == AMEDIA_OK) {
                    AMediaCodec_start(m_audioCodec);
                    m_hasAudio = true;
                } else {
                    LOGE("Failed to configure audio codec");
                }
                AMediaFormat_delete(format);
            }
        }

        m_isRecording = true;
        m_startTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        LOGI("MediaEncoder successfully started. Video: %d, Audio: %d", m_hasVideo, m_hasAudio);
        return true;
    }

    void stop() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_isRecording) return;
        m_isRecording = false;

        if (m_videoCodec) {
            AMediaCodec_signalEndOfInputStream(m_videoCodec);
        }

        // Feed EOS to audio
        if (m_audioCodec) {
             ssize_t inputBufIdx = AMediaCodec_dequeueInputBuffer(m_audioCodec, 10000);
             if (inputBufIdx >= 0) {
                 AMediaCodec_queueInputBuffer(m_audioCodec, inputBufIdx, 0, 0, 0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
             }
        }

        // Drain with a small timeout to allow EOS packets to propagate
        drainInternal(10000);

        if (m_videoCodec) {
            AMediaCodec_stop(m_videoCodec);
            AMediaCodec_delete(m_videoCodec);
            m_videoCodec = nullptr;
        }

        if (m_audioCodec) {
            AMediaCodec_stop(m_audioCodec);
            AMediaCodec_delete(m_audioCodec);
            m_audioCodec = nullptr;
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

    void encodeAudioFrame(const int16_t* audioData, int numFrames, int64_t timestampNs) override {
        if (!m_isRecording || !m_audioCodec) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        ssize_t inputBufIdx = AMediaCodec_dequeueInputBuffer(m_audioCodec, 0);
        if (inputBufIdx >= 0) {
            size_t bufSize = 0;
            uint8_t* buf = AMediaCodec_getInputBuffer(m_audioCodec, inputBufIdx, &bufSize);
            size_t dataSize = numFrames * 2 * sizeof(int16_t); // 2 channels

            if (buf && dataSize <= bufSize) {
                memcpy(buf, audioData, dataSize);
                // Convert timestamp to microseconds relative to start
                int64_t ptsUs = (timestampNs - m_startTimeNs) / 1000;
                if (ptsUs < 0) ptsUs = 0;
                AMediaCodec_queueInputBuffer(m_audioCodec, inputBufIdx, 0, dataSize, ptsUs, 0);
            }
        }
        drainInternal();
    }

    void drain() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        drainInternal(0);
    }

    void* getInputWindow() override { return m_window; }

    void notifyFrameReady() override {
        drain();
    }

private:
    void drainInternal(int64_t timeoutUs = 0) {
        if (!m_muxer) return;

        bool videoDone = !m_hasVideo;
        bool audioDone = !m_hasAudio;

        // Ensure both tracks are added before starting muxer
        if (!m_muxerStarted) {
            if (m_hasVideo && m_videoTrackIndex < 0) {
                AMediaCodecBufferInfo info;
                ssize_t status = AMediaCodec_dequeueOutputBuffer(m_videoCodec, &info, 0);
                if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                    AMediaFormat* format = AMediaCodec_getOutputFormat(m_videoCodec);
                    m_videoTrackIndex = AMediaMuxer_addTrack(m_muxer, format);
                    AMediaFormat_delete(format);
                } else if (status >= 0) {
                    AMediaCodec_releaseOutputBuffer(m_videoCodec, status, false);
                }
            }
            if (m_hasAudio && m_audioTrackIndex < 0) {
                AMediaCodecBufferInfo info;
                ssize_t status = AMediaCodec_dequeueOutputBuffer(m_audioCodec, &info, 0);
                if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                    AMediaFormat* format = AMediaCodec_getOutputFormat(m_audioCodec);
                    m_audioTrackIndex = AMediaMuxer_addTrack(m_muxer, format);
                    AMediaFormat_delete(format);
                } else if (status >= 0) {
                    AMediaCodec_releaseOutputBuffer(m_audioCodec, status, false);
                }
            }

            if ((!m_hasVideo || m_videoTrackIndex >= 0) && (!m_hasAudio || m_audioTrackIndex >= 0)) {
                AMediaMuxer_start(m_muxer);
                m_muxerStarted = true;
            }
        }

        if (!m_muxerStarted) return;

        // Drain Video
        if (m_hasVideo) {
            while (true) {
                AMediaCodecBufferInfo info;
                ssize_t status = AMediaCodec_dequeueOutputBuffer(m_videoCodec, &info, timeoutUs);
                if (status >= 0) {
                    uint8_t* buf = AMediaCodec_getOutputBuffer(m_videoCodec, status, nullptr);
                    if (buf && info.size > 0) {
                        // HW Encoders output absolute PTS (device uptime). We must offset it to start at 0.
                        int64_t ptsUs = info.presentationTimeUs - (m_startTimeNs / 1000);
                        if (ptsUs < 0) ptsUs = 0;
                        info.presentationTimeUs = ptsUs;
                        AMediaMuxer_writeSampleData(m_muxer, m_videoTrackIndex, buf, &info);
                    }
                    AMediaCodec_releaseOutputBuffer(m_videoCodec, status, false);
                    if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) break;
                } else {
                    break;
                }
            }
        }

        // Drain Audio
        if (m_hasAudio) {
            while (true) {
                AMediaCodecBufferInfo info;
                ssize_t status = AMediaCodec_dequeueOutputBuffer(m_audioCodec, &info, timeoutUs);
                if (status >= 0) {
                    uint8_t* buf = AMediaCodec_getOutputBuffer(m_audioCodec, status, nullptr);
                    if (buf && info.size > 0) {
                        AMediaMuxer_writeSampleData(m_muxer, m_audioTrackIndex, buf, &info);
                    }
                    AMediaCodec_releaseOutputBuffer(m_audioCodec, status, false);
                    if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) break;
                } else {
                    break;
                }
            }
        }
    }

    bool m_isRecording = false;
    bool m_muxerStarted = false;
    bool m_hasVideo = false;
    bool m_hasAudio = false;
    ssize_t m_videoTrackIndex = -1;
    ssize_t m_audioTrackIndex = -1;

    AMediaCodec* m_videoCodec = nullptr;
    AMediaCodec* m_audioCodec = nullptr;
    AMediaMuxer* m_muxer = nullptr;
    ANativeWindow* m_window = nullptr;

    int64_t m_startTimeNs = 0;
    std::mutex m_mutex;
};

std::shared_ptr<MediaEncoder> MediaEncoder::create() {
    return std::make_shared<NdkMediaEncoder>();
}

} // namespace vfx
#else
namespace vfx {
class MockMediaEncoder : public MediaEncoder {
public:
    bool start(const std::string& outputPath, int width, int height, VideoCodecType requestedCodec, bool enableAudio) override { return true; }
    void stop() override {}
    void drain() override {}
    void encodeAudioFrame(const int16_t* audioData, int numFrames, int64_t timestampNs) override {}
    void* getInputWindow() override { return nullptr; }
    void notifyFrameReady() override {}
};
std::shared_ptr<MediaEncoder> MediaEncoder::create() {
    return std::make_shared<MockMediaEncoder>();
}
}
#endif
