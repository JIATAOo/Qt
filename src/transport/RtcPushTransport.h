#ifndef __RTC_PUSH_TRANSPORT_H__
#define __RTC_PUSH_TRANSPORT_H__

#include "RtcTransportBase.h"
#include "codec/video_codec/VideoCodecFactory.h"
#include "codec/audio_codec/AudioCodecFactory.h"
#include <memory>
#include <mutex>
#include <atomic>

namespace TRANSPORT
{
    class RtcPushTransport : public RtcTransportBase
    {
    public:
        RtcPushTransport();
        ~RtcPushTransport() override;

    public:
        bool StartPublishVideo(const std::string& cname);
        bool StopPublishVideo();
        bool StartPublishAudio(const std::string& cname);
        bool StopPublishAudio();

        bool PushVideoFrame(const std::shared_ptr<I420Frame>& frame);
        bool PushAudioFrame(const std::shared_ptr<AudioFrame>& frame);

        bool IsPublishingVideo() const { return m_video_send_enabled; }
        bool IsPublishingAudio() const { return m_audio_send_enabled; }

    private:
        bool SetupPublishTracks(const std::string& cname);
        bool InitVideoEncoder();
        bool InitAudioEncoder();

        CODEC::EncodedVideoFrame EncodeVideoFrame(const std::shared_ptr<I420Frame>& frame);

    public:
        void Close() override;

    private:
        std::atomic<bool> m_video_send_enabled{false};
        std::atomic<bool> m_audio_send_enabled{false};

        std::shared_ptr<rtc::Track> m_camera_track;
        std::shared_ptr<rtc::Track> m_audio_track;

        mutable std::mutex m_tracks_mutex;

        std::atomic<uint64_t> m_session_start_time_us{0};

        std::unique_ptr<CODEC::IVideoEncoder> m_video_encoder;
        std::unique_ptr<CODEC::IAudioEncoder> m_audio_encoder;

        mutable std::mutex m_video_encoder_mutex;
        mutable std::mutex m_audio_encoder_mutex;
    };

}

#endif // __RTC_PUSH_TRANSPORT_H__
