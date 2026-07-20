#ifndef __RTC_PULL_TRANSPORT_H__
#define __RTC_PULL_TRANSPORT_H__

#include "RtcTransportBase.h"
#include "codec/video_codec/VideoCodecFactory.h"
#include "codec/audio_codec/AudioCodecFactory.h"
#include <memory>
#include <mutex>
#include <cstdint>
#include <atomic>

namespace rtc {
    class RtcpReceivingSession;
}

namespace TRANSPORT
{

    class RtcPullTransport : public RtcTransportBase
    {
    public:
        RtcPullTransport();
        ~RtcPullTransport() override;

    public:
        bool SubscribeAudioVideo();

        void SetVideoFrameCallback(OnVideoFrameCallback cb) { m_video_frame_callback = cb; }
        void SetAudioFrameCallback(OnAudioFrameCallback cb) { m_audio_frame_callback = cb; }

    private:
        bool InitVideoDecoder();
        bool InitAudioDecoder();

        void ProcessIncomingVideoData(const uint8_t* data, size_t size, uint64_t timestamp);
        bool DecodeVideoFrame(const CODEC::EncodedVideoFrame& encoded_frame);
        void ProcessIncomingAudioData(const uint8_t* data, size_t size, uint64_t timestamp);
        bool DecodeAudioFrame(const CODEC::EncodedAudioFrame& encoded_frame);

        // Timebase state for local clock-based AV synchronization
        std::atomic<bool> m_video_ts_initialized{false};
        uint32_t m_video_base_rtp{0};
        uint64_t m_video_base_time_us{0};

        std::atomic<bool> m_audio_ts_initialized{false};
        uint32_t m_audio_base_rtp{0};
        uint64_t m_audio_base_time_us{0};

    public:
        void Close() override;

    private:
        std::shared_ptr<rtc::Track> m_recv_video_track;
        std::shared_ptr<rtc::Track> m_recv_audio_track;

        mutable std::mutex m_tracks_mutex;

        OnVideoFrameCallback m_video_frame_callback;
        OnAudioFrameCallback m_audio_frame_callback;

        std::unique_ptr<CODEC::IVideoDecoder> m_video_decoder;
        std::unique_ptr<CODEC::IAudioDecoder> m_audio_decoder;

        CODEC::VideoCodecConfig m_video_decoder_config;
        CODEC::AudioCodecConfig m_audio_decoder_config;

        mutable std::mutex m_video_decoder_mutex;
        mutable std::mutex m_audio_decoder_mutex;
    };

}

#endif // __RTC_PULL_TRANSPORT_H__
