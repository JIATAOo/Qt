#include "RtcPullTransport.h"
#include "codec/video_codec/VideoCodecInterface.h"
#include "codec/audio_codec/AudioCodecInterface.h"
#include "Logger.h"
#include <cstring>
#include <chrono>

#include "rtc/h264rtpdepacketizer.hpp"
#include "rtc/rtpdepacketizer.hpp"
#include "rtc/frameinfo.hpp"
#include "rtc/rtcpreceivingsession.hpp"

#define OUTPUT_PULL_LOG 0

namespace TRANSPORT
{

RtcPullTransport::RtcPullTransport() = default;

RtcPullTransport::~RtcPullTransport()
{
    Close();
}

bool RtcPullTransport::InitVideoDecoder()
{
    std::lock_guard<std::mutex> lock(m_video_decoder_mutex);

    m_video_decoder = CODEC::VideoCodecFactory::Instance().CreateDecoder(CODEC::VideoCodecType::H264);
    if (!m_video_decoder)
    {
        LOG_ERROR("Failed to create video decoder");
        return false;
    }

    int default_bitrate = m_publish_info.video_width * m_publish_info.video_height * m_publish_info.video_fps / 8000;
    default_bitrate = std::max(200, std::min(default_bitrate, 6000));

    CODEC::VideoCodecConfig config;
    config.width = m_publish_info.video_width;
    config.height = m_publish_info.video_height;
    config.framerate = m_publish_info.video_fps;
    config.target_bitrate_kbps = default_bitrate;
    config.max_bitrate_kbps = default_bitrate;

    if (!m_video_decoder->Initialize(config))
    {
        LOG_ERROR("Failed to initialize video decoder");
        m_video_decoder.reset();
        return false;
    }

    m_video_decoder_config = config;
    LOG_INFO("Video decoder created: H264");
    return true;
}

bool RtcPullTransport::InitAudioDecoder()
{
    std::lock_guard<std::mutex> lock(m_audio_decoder_mutex);

    m_audio_decoder = CODEC::AudioCodecFactory::Instance().CreateDecoder(CODEC::AudioCodecType::OPUS);
    if (!m_audio_decoder)
    {
        LOG_ERROR("Failed to create audio decoder");
        return false;
    }

    int default_bitrate = m_publish_info.audio_channels * 64;

    CODEC::AudioCodecConfig config;
    config.sample_rate = m_publish_info.audio_sample_rate;
    config.channels = m_publish_info.audio_channels;
    config.bitrate_kbps = default_bitrate;
    config.frame_size_ms = 10;
    config.enable_vbr = true;
    config.enable_fec = true;

    if (!m_audio_decoder->Initialize(config))
    {
        LOG_ERROR("Failed to initialize audio decoder");
        m_audio_decoder.reset();
        return false;
    }

    m_audio_decoder_config = config;
    LOG_INFO("Audio decoder created: OPUS");
    return true;
}

bool RtcPullTransport::SubscribeAudioVideo()
{
    if (!m_rtc_connection) return false;

    LOG_INFO("========== SubscribeAudioVideo ==========");

    if (!InitVideoDecoder())
    {
        LOG_INFO("Failed to create H264 video decoder");
        return false;
    }

    if (!InitAudioDecoder())
    {
        LOG_INFO("Failed to create Opus audio decoder");
        return false;
    }

    rtc::Description::Video video_desc("video", rtc::Description::Direction::RecvOnly);
    video_desc.addH264Codec(RTP_PAYLOAD_TYPE_H264,
        "profile-level-id=42e01f;packetization-mode=1;level-asymmetry-allowed=1;max-mbps=3600;max-fs=3600");
    m_recv_video_track = m_rtc_connection->addTrack(video_desc);

    if (m_recv_video_track)
    {
        auto video_depacketizer = std::make_shared<rtc::H264RtpDepacketizer>();
        m_recv_video_track->setMediaHandler(video_depacketizer);
        m_recv_video_track->onOpen([this]() {
            LOG_INFO("Recv Video track OPEN");
            if (m_recv_video_track) m_recv_video_track->requestKeyframe();
        });
        m_recv_video_track->onClosed([this]() {
            LOG_INFO("Recv Video track CLOSED");
        });
        m_recv_video_track->onError([](std::string error) {
            LOG_INFO_STREAM << "Recv Video track ERROR: " << error;
        });

        m_recv_video_track->onFrame([this](rtc::binary message, rtc::FrameInfo info) {
            uint64_t timestamp_us = 0;
            if (!m_video_ts_initialized)
            {
                m_video_base_rtp = info.timestamp;
                m_video_base_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                m_video_ts_initialized = true;
                timestamp_us = m_video_base_time_us;
            }
            else
            {
                int32_t rtp_delta = static_cast<int32_t>(info.timestamp - m_video_base_rtp);
                int64_t elapsed_us = static_cast<int64_t>(rtp_delta) * 1000000LL / 90000;
                timestamp_us = static_cast<uint64_t>(static_cast<int64_t>(m_video_base_time_us) + elapsed_us);
            }

            const uint8_t* data = reinterpret_cast<const uint8_t*>(message.data());
            ProcessIncomingVideoData(data, message.size(), timestamp_us);
        });
    }

    rtc::Description::Audio audio_desc("audio", rtc::Description::Direction::RecvOnly);
    audio_desc.addOpusCodec(RTP_PAYLOAD_TYPE_OPUS);
    m_recv_audio_track = m_rtc_connection->addTrack(audio_desc);

    if (m_recv_audio_track)
    {
        auto audio_depacketizer = std::make_shared<rtc::RtpDepacketizer>(48000);
        m_recv_audio_track->setMediaHandler(audio_depacketizer);

        m_recv_audio_track->onOpen([this]() {
            LOG_INFO("Recv Audio track OPEN");
        });
        m_recv_audio_track->onClosed([this]() {
            LOG_INFO("Recv Audio track CLOSED");
        });
        m_recv_audio_track->onError([](std::string error) {
            LOG_INFO_STREAM << "Recv Audio track ERROR: " << error;
        });

        m_recv_audio_track->onFrame([this](rtc::binary message, rtc::FrameInfo info) {
            uint64_t timestamp_us = 0;
            if (!m_audio_ts_initialized)
            {
                m_audio_base_rtp = info.timestamp;
                m_audio_base_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                m_audio_ts_initialized = true;
                timestamp_us = m_audio_base_time_us;
            }
            else
            {
                int32_t rtp_delta = static_cast<int32_t>(info.timestamp - m_audio_base_rtp);
                int64_t elapsed_us = static_cast<int64_t>(rtp_delta) * 1000000LL / 48000;
                timestamp_us = static_cast<uint64_t>(static_cast<int64_t>(m_audio_base_time_us) + elapsed_us);
            }

            const uint8_t* data = reinterpret_cast<const uint8_t*>(message.data());
            ProcessIncomingAudioData(data, message.size(), timestamp_us);
        });
    }

    m_rtc_connection->setLocalDescription(rtc::Description::Type::Offer);
    LOG_INFO("setLocalDescription(Offer) called for subscribe flow");
    LOG_INFO("==========================================");

    return m_recv_video_track != nullptr && m_recv_audio_track != nullptr;
}

void RtcPullTransport::ProcessIncomingVideoData(const uint8_t* data, size_t size, uint64_t timestamp)
{
    CODEC::EncodedVideoFrame encoded_frame;
    encoded_frame.data.assign(data, data + size);
    encoded_frame.timestamp_us = timestamp;
    encoded_frame.width = 0;
    encoded_frame.height = 0;
    encoded_frame.codec_type = CODEC::VideoCodecType::H264;

    DecodeVideoFrame(encoded_frame);
}

bool RtcPullTransport::DecodeVideoFrame(const CODEC::EncodedVideoFrame& encoded_frame)
{
    CODEC::RawVideoFrame raw_frame;

    bool needs_keyframe = false;
    {
        std::lock_guard<std::mutex> lock(m_video_decoder_mutex);
        if (!m_video_decoder) return false;

        if (!m_video_decoder->Decode(encoded_frame, raw_frame))
        {
            needs_keyframe = m_video_decoder->NeedsKeyframe();
            return false;
        }
    }

    if (needs_keyframe)
    {
        std::lock_guard<std::mutex> track_lock(m_tracks_mutex);
        if (m_recv_video_track && m_recv_video_track->isOpen())
        {
            if (m_recv_video_track->requestKeyframe())
            {
                std::lock_guard<std::mutex> lock(m_video_decoder_mutex);
                if (m_video_decoder)
                {
                    m_video_decoder->ClearKeyframeRequest();
                }
            }
        }
        return false;
    }

    if (m_video_frame_callback)
    {
        auto frame = std::make_shared<I420Frame>();
        frame->width = raw_frame.width;
        frame->height = raw_frame.height;
        frame->format = static_cast<int>(raw_frame.format);
        frame->timestamp_us = raw_frame.timestamp_us;

        int y_stride = raw_frame.stride[0] > 0 ? raw_frame.stride[0] : raw_frame.width;
        int u_stride = raw_frame.stride[1] > 0 ? raw_frame.stride[1] : raw_frame.width / 2;
        int v_stride = raw_frame.stride[2] > 0 ? raw_frame.stride[2] : raw_frame.width / 2;

        size_t y_plane_size = raw_frame.width * raw_frame.height;
        size_t uv_plane_size = raw_frame.width * raw_frame.height / 4;

        std::shared_ptr<uint8_t[]> y_buffer(new uint8_t[y_plane_size]);
        std::shared_ptr<uint8_t[]> u_buffer(new uint8_t[uv_plane_size]);
        std::shared_ptr<uint8_t[]> v_buffer(new uint8_t[uv_plane_size]);

        uint8_t *y_src = raw_frame.data[0].get();
        for (uint32_t i = 0; i < raw_frame.height; ++i)
        {
            std::memcpy(y_buffer.get() + i * raw_frame.width,
                        y_src + i * y_stride, raw_frame.width);
        }

        uint32_t uv_height = raw_frame.height / 2;
        uint32_t uv_width = raw_frame.width / 2;
        uint8_t *u_src = raw_frame.data[1].get();
        for (uint32_t i = 0; i < uv_height; ++i)
        {
            std::memcpy(u_buffer.get() + i * uv_width,
                        u_src + i * u_stride, uv_width);
        }

        uint8_t *v_src = raw_frame.data[2].get();
        for (uint32_t i = 0; i < uv_height; ++i)
        {
            std::memcpy(v_buffer.get() + i * uv_width,
                        v_src + i * v_stride, uv_width);
        }

        frame->data[0] = y_buffer;
        frame->data[1] = u_buffer;
        frame->data[2] = v_buffer;

        m_video_frame_callback(frame);
    }

    return true;
}

void RtcPullTransport::ProcessIncomingAudioData(const uint8_t* data, size_t size, uint64_t timestamp)
{
    CODEC::EncodedAudioFrame encoded_frame;
    encoded_frame.data.assign(data, data + size);
    encoded_frame.timestamp_us = timestamp;
    encoded_frame.sample_count = 0;
    encoded_frame.codec_type = CODEC::AudioCodecType::OPUS;

    DecodeAudioFrame(encoded_frame);
}

bool RtcPullTransport::DecodeAudioFrame(const CODEC::EncodedAudioFrame& encoded_frame)
{
    std::lock_guard<std::mutex> lock(m_audio_decoder_mutex);
    if (!m_audio_decoder) return false;

    if (!m_audio_decoder->PushInput(encoded_frame)) return false;

    CODEC::RawAudioFrame raw_frame;
    while (m_audio_decoder->PullDecoded(raw_frame))
    {
        if (m_audio_frame_callback)
        {
            auto frame = std::make_shared<AudioFrame>();
            frame->sample_rate = raw_frame.sample_rate;
            frame->channels = raw_frame.channels;
            frame->format = static_cast<int>(raw_frame.format);
            frame->timestamp_us = raw_frame.timestamp_us;
            frame->samples = raw_frame.sample_per_channel;
            frame->data = raw_frame.data;

            m_audio_frame_callback(frame);
        }
    }

    return true;
}

void RtcPullTransport::Close()
{
    {
        std::lock_guard<std::mutex> lock(m_tracks_mutex);
        if (m_recv_video_track) { m_recv_video_track->close(); m_recv_video_track.reset(); }
        if (m_recv_audio_track) { m_recv_audio_track->close(); m_recv_audio_track.reset(); }
    }

    {
        std::lock_guard<std::mutex> lock(m_video_decoder_mutex);
        if (m_video_decoder) { m_video_decoder->Release(); m_video_decoder.reset(); }
    }

    {
        std::lock_guard<std::mutex> lock(m_audio_decoder_mutex);
        if (m_audio_decoder) { m_audio_decoder->Release(); m_audio_decoder.reset(); }
    }

    if (m_rtc_connection) { m_rtc_connection->close(); m_rtc_connection.reset(); }

    SetState(ConnectionState::kDisconnected);
}

}
