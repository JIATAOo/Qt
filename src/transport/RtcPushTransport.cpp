#include "RtcPushTransport.h"
#include "codec/video_codec/VideoCodecInterface.h"
#include "codec/audio_codec/AudioCodecInterface.h"
#include "Logger.h"
#include <sstream>
#include <random>
#include <chrono>
#include <memory>

#include "rtc/rtc.hpp"
#include "rtc/h264rtppacketizer.hpp"
#include "rtc/rtppacketizationconfig.hpp"
#include "rtc/rtcpsrreporter.hpp"
#include "rtc/rtcpnackresponder.hpp"
#include "rtc/plihandler.hpp"
#include "rtc/frameinfo.hpp"

#define OUTPUT_SEND_LOG 0

namespace TRANSPORT
{

static std::string GenerateUUID()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

static uint32_t GenerateRandomSSRC()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(1, UINT32_MAX);
    return dis(gen);
}

RtcPushTransport::RtcPushTransport()
{
}

RtcPushTransport::~RtcPushTransport()
{
    Close();
}

bool RtcPushTransport::InitVideoEncoder()
{
    std::lock_guard<std::mutex> lock(m_video_encoder_mutex);

    m_video_encoder = CODEC::VideoCodecFactory::Instance().CreateEncoder(CODEC::VideoCodecType::H264);
    if (!m_video_encoder)
    {
        LOG_ERROR("Failed to create video encoder");
        return false;
    }

    int default_bitrate = m_publish_info.video_width * m_publish_info.video_height * m_publish_info.video_fps / 8000;
    default_bitrate = std::max(200, std::min(default_bitrate, 6000));

    CODEC::VideoCodecConfig config;
    config.width = m_publish_info.video_width;
    config.height = m_publish_info.video_height;
    config.framerate = m_publish_info.video_fps;
    config.target_bitrate_kbps = default_bitrate;
    config.max_bitrate_kbps = default_bitrate * 3 / 2;
    config.keyframe_interval = 60;

    if (!m_video_encoder->Initialize(config))
    {
        LOG_ERROR("Failed to initialize video encoder");
        m_video_encoder.reset();
        return false;
    }

    LOG_INFO("Video encoder created: H264");
    return true;
}

bool RtcPushTransport::InitAudioEncoder()
{
    std::lock_guard<std::mutex> lock(m_audio_encoder_mutex);

    m_audio_encoder = CODEC::AudioCodecFactory::Instance().CreateEncoder(CODEC::AudioCodecType::OPUS);
    if (!m_audio_encoder)
    {
        LOG_ERROR("Failed to create audio encoder");
        return false;
    }

    int default_bitrate = m_publish_info.audio_channels * 64;

    CODEC::AudioCodecConfig config;
    config.sample_rate = m_publish_info.audio_sample_rate;
    config.channels = m_publish_info.audio_channels;
    config.bitrate_kbps = default_bitrate;
    config.frame_size_ms = 20;
    config.enable_vbr = true;
    config.enable_fec = true;

    if (!m_audio_encoder->Initialize(config))
    {
        LOG_ERROR("Failed to initialize audio encoder");
        m_audio_encoder.reset();
        return false;
    }

    LOG_INFO("Audio encoder created: OPUS");
    return true;
}

bool RtcPushTransport::StartPublishVideo(const std::string& cname)
{
    if (!m_camera_track && !SetupPublishTracks(cname))
    {
        return false;
    }

    if (!InitVideoEncoder())
    {
        return false;
    }

    m_video_send_enabled = true;
    return true;
}

bool RtcPushTransport::StopPublishVideo()
{
    m_video_send_enabled = false;
    return true;
}

bool RtcPushTransport::StartPublishAudio(const std::string& cname)
{
    if (!m_audio_track && !SetupPublishTracks(cname))
    {
        return false;
    }

    if (!InitAudioEncoder())
    {
        return false;
    }

    m_audio_send_enabled = true;
    return true;
}

bool RtcPushTransport::StopPublishAudio()
{
    if (m_audio_send_enabled)
    {
        std::lock_guard<std::mutex> lock(m_audio_encoder_mutex);
        if (m_audio_encoder)
        {
            m_audio_encoder->Flush();
        }
        
        CODEC::EncodedAudioFrame encoded_frame;
        while (m_audio_encoder && m_audio_encoder->PullEncoded(encoded_frame))
        {
            std::lock_guard<std::mutex> track_lock(m_tracks_mutex);
            if (m_audio_track && m_audio_track->isOpen())
            {
                rtc::binary sample(
                    reinterpret_cast<const std::byte *>(encoded_frame.data.data()),
                    reinterpret_cast<const std::byte *>(encoded_frame.data.data() + encoded_frame.data.size()));

                uint64_t current_time_us = encoded_frame.timestamp_us;
                {
                    uint64_t expected = 0;
                    m_session_start_time_us.compare_exchange_strong(expected, current_time_us);
                }

                uint64_t elapsed_us = current_time_us - m_session_start_time_us.load();
                double elapsed_seconds = static_cast<double>(elapsed_us) / 1000000.0;

                rtc::FrameInfo frame_info(0u);
                frame_info.timestampSeconds = std::chrono::duration<double>(elapsed_seconds);
                frame_info.payloadType = RTP_PAYLOAD_TYPE_OPUS;

                m_audio_track->sendFrame(sample, frame_info);
            }
        }
    }
    
    m_audio_send_enabled = false;
    return true;
}

bool RtcPushTransport::PushVideoFrame(const std::shared_ptr<I420Frame>& frame)
{
    if (!m_video_send_enabled)
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_tracks_mutex);
        if (!m_camera_track || m_camera_track->isClosed())
        {
            return false;
        }
    }

    CODEC::EncodedVideoFrame encoded_frame = EncodeVideoFrame(frame);
    if (encoded_frame.is_skip)
    {
        return true;
    }

    std::lock_guard<std::mutex> lock(m_tracks_mutex);
    if (!m_camera_track || !m_camera_track->isOpen())
    {
        return false;
    }

    rtc::binary sample(
        reinterpret_cast<const std::byte*>(encoded_frame.data.data()),
        reinterpret_cast<const std::byte*>(encoded_frame.data.data() + encoded_frame.data.size()));

    uint64_t current_time_us = encoded_frame.timestamp_us;

    {
        uint64_t expected = 0;
        m_session_start_time_us.compare_exchange_strong(expected, current_time_us);
    }

    uint64_t elapsed_us = current_time_us - m_session_start_time_us.load();
    double elapsed_seconds = static_cast<double>(elapsed_us) / 1000000.0;

    rtc::FrameInfo frame_info(0u);
    frame_info.timestampSeconds = std::chrono::duration<double>(elapsed_seconds);
    frame_info.payloadType = RTP_PAYLOAD_TYPE_H264;

#if OUTPUT_SEND_LOG
    {
        static int s_vfc = 0;
        if (s_vfc % 30 == 0)
        {
            LOG_DEBUG_STREAM << "[Send Video] elapsed_us=" << elapsed_us << " rtp_sec=" << elapsed_seconds;
        }
        ++s_vfc;
    }
#endif

    m_camera_track->sendFrame(sample, frame_info);
    return true;
}

bool RtcPushTransport::PushAudioFrame(const std::shared_ptr<AudioFrame>& frame)
{
    if (!m_audio_send_enabled)
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_tracks_mutex);
        if (!m_audio_track || m_audio_track->isClosed())
        {
            return false;
        }
    }

    CODEC::RawAudioFrame raw_frame;
    raw_frame.sample_rate = frame->sample_rate;
    raw_frame.channels = frame->channels;
    raw_frame.format = CODEC::AudioSampleFormat::PCM_FLOAT;
    raw_frame.timestamp_us = frame->timestamp_us;
    raw_frame.sample_per_channel = frame->samples;
    raw_frame.data = frame->data;

    {
        std::lock_guard<std::mutex> lock(m_audio_encoder_mutex);
        if (!m_audio_encoder || !m_audio_encoder->PushInput(raw_frame))
        {
            return false;
        }

        CODEC::EncodedAudioFrame encoded_frame;
        while (m_audio_encoder->PullEncoded(encoded_frame))
        {
            std::lock_guard<std::mutex> track_lock(m_tracks_mutex);
            if (!m_audio_track || !m_audio_track->isOpen())
            {
                return false;
            }

            rtc::binary sample(
                reinterpret_cast<const std::byte*>(encoded_frame.data.data()),
                reinterpret_cast<const std::byte*>(encoded_frame.data.data() + encoded_frame.data.size()));

            uint64_t current_time_us = frame->timestamp_us;
            {
                uint64_t expected = 0;
                m_session_start_time_us.compare_exchange_strong(expected, current_time_us);
            }

            uint64_t elapsed_us = current_time_us - m_session_start_time_us.load();
            double elapsed_seconds = static_cast<double>(elapsed_us) / 1000000.0;

            rtc::FrameInfo frame_info(0u);
            frame_info.timestampSeconds = std::chrono::duration<double>(elapsed_seconds);
            frame_info.payloadType = RTP_PAYLOAD_TYPE_OPUS;

#if OUTPUT_SEND_LOG
            {
                static int s_afc = 0;
                if (s_afc % 50 == 0)
                {
                    LOG_DEBUG_STREAM << "[Send Audio] elapsed_us=" << elapsed_us << " rtp_sec=" << elapsed_seconds;
                }
                ++s_afc;
            }
#endif
            m_audio_track->sendFrame(sample, frame_info);
        }
    }

    return true;
}

bool RtcPushTransport::SetupPublishTracks(const std::string& cname)
{
    if (!m_rtc_connection)
    {
        return false;
    }

    rtc::Description::Video local_video("1", rtc::Description::Direction::SendOnly);
    local_video.addH264Codec(RTP_PAYLOAD_TYPE_H264,
        "profile-level-id=42e01f;packetization-mode=1;level-asymmetry-allowed=1;max-mbps=3600;max-fs=3600");
    uint32_t video_ssrc = GenerateRandomSSRC();
    local_video.addSSRC(video_ssrc, cname, GenerateUUID(), GenerateUUID());
    m_camera_track = m_rtc_connection->addTrack(local_video);

    auto video_rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(
        video_ssrc, cname, RTP_PAYLOAD_TYPE_H264, rtc::H264RtpPacketizer::ClockRate);
    auto video_packetizer = std::make_shared<rtc::H264RtpPacketizer>(
        rtc::H264RtpPacketizer::Separator::LongStartSequence, video_rtp_config);
    auto video_sr = std::make_shared<rtc::RtcpSrReporter>(video_rtp_config);
    video_packetizer->addToChain(video_sr);
    auto video_nack = std::make_shared<rtc::RtcpNackResponder>();
    video_packetizer->addToChain(video_nack);
    auto video_pli_handler = std::make_shared<rtc::PliHandler>([this, pli_count = 0]() mutable
    {
        if (!m_video_send_enabled.load()) return;
        ++pli_count;
        LOG_INFO_STREAM << "PliHandler #" << pli_count << " - requesting keyframe";
        std::lock_guard<std::mutex> lock(m_video_encoder_mutex);
        if (m_video_encoder)
        {
            m_video_encoder->RequestKeyframe();
        } 
    });
    video_packetizer->addToChain(video_pli_handler);
    m_camera_track->setMediaHandler(video_packetizer);

    m_camera_track->onOpen([this]() {
        LOG_INFO("Video track OPEN");
    });
    m_camera_track->onClosed([this]() {
        LOG_INFO("Video track CLOSED");
    });
    m_camera_track->onError([](std::string error) {
        LOG_INFO_STREAM << "Video track ERROR: " << error;
    });

    rtc::Description::Audio local_audio("0", rtc::Description::Direction::SendOnly);
    local_audio.addOpusCodec(RTP_PAYLOAD_TYPE_OPUS);
    uint32_t audio_ssrc = GenerateRandomSSRC();
    local_audio.addSSRC(audio_ssrc, cname, GenerateUUID(), GenerateUUID());
    m_audio_track = m_rtc_connection->addTrack(local_audio);

    auto audio_rtp_config = std::make_shared<rtc::RtpPacketizationConfig>(
        audio_ssrc, cname, RTP_PAYLOAD_TYPE_OPUS, rtc::OpusRtpPacketizer::DefaultClockRate);
    auto audio_packetizer = std::make_shared<rtc::OpusRtpPacketizer>(audio_rtp_config);
    auto audio_sr = std::make_shared<rtc::RtcpSrReporter>(audio_rtp_config);
    audio_packetizer->addToChain(audio_sr);
    auto audio_nack = std::make_shared<rtc::RtcpNackResponder>();
    audio_packetizer->addToChain(audio_nack);
    m_audio_track->setMediaHandler(audio_packetizer);

    m_audio_track->onOpen([this]() {
        LOG_INFO("Audio track OPEN");
    });
    m_audio_track->onClosed([this]() {
        LOG_INFO("Audio track CLOSED");
    });
    m_audio_track->onError([](std::string error) {
        LOG_INFO_STREAM << "Audio track ERROR: " << error;
    });

    m_rtc_connection->setLocalDescription(rtc::Description::Type::Offer);    
    return true;
}

CODEC::EncodedVideoFrame RtcPushTransport::EncodeVideoFrame(const std::shared_ptr<I420Frame>& frame)
{
    CODEC::RawVideoFrame raw_frame;
    raw_frame.width = frame->width;
    raw_frame.height = frame->height;
    raw_frame.format = CODEC::VideoFormat::I420;
    raw_frame.timestamp_us = frame->timestamp_us;
    raw_frame.stride[0] = frame->width;
    raw_frame.stride[1] = frame->width / 2;
    raw_frame.stride[2] = frame->width / 2;

    raw_frame.data[0] = frame->data[0];
    raw_frame.data[1] = frame->data[1];
    raw_frame.data[2] = frame->data[2]; 

    CODEC::EncodedVideoFrame encoded_frame;
    std::lock_guard<std::mutex> lock(m_video_encoder_mutex);
    if (m_video_encoder)
    {
        m_video_encoder->Encode(raw_frame, encoded_frame);
    }
    return encoded_frame;
}

void RtcPushTransport::Close()
{
    m_video_send_enabled = false;
    m_audio_send_enabled = false;

    {
        std::lock_guard<std::mutex> lock(m_tracks_mutex);
        if (m_camera_track) { m_camera_track->close(); m_camera_track.reset(); }
        if (m_audio_track) { m_audio_track->close(); m_audio_track.reset(); }
    }

    m_session_start_time_us.store(0);

    {
        std::lock_guard<std::mutex> lock(m_video_encoder_mutex);
        if (m_video_encoder) { m_video_encoder->Release(); m_video_encoder.reset(); }
    }

    {
        std::lock_guard<std::mutex> lock(m_audio_encoder_mutex);
        if (m_audio_encoder) { m_audio_encoder->Release(); m_audio_encoder.reset(); }
    }

    if (m_rtc_connection) { m_rtc_connection->close(); m_rtc_connection.reset(); }

    SetState(ConnectionState::kDisconnected);
}

}
