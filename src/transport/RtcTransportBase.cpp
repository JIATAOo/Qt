#include "RtcTransportBase.h"
#include "Logger.h"

namespace TRANSPORT
{

RtcTransportBase::RtcTransportBase()
{
}

RtcTransportBase::~RtcTransportBase()
{
}

bool RtcTransportBase::Initialize(const PublishInfo& info)
{
    m_publish_info = info;
    return true;
}

bool RtcTransportBase::Open()
{
    if (m_rtc_connection)
    {
        return true;
    }

    LOG_INFO("========== RtcTransportBase::Open ==========");
    LOG_INFO_STREAM << "Video: " << m_publish_info.video_width << "x" << m_publish_info.video_height << " @" << m_publish_info.video_fps << "fps";
    LOG_INFO_STREAM << "Audio: " << m_publish_info.audio_sample_rate << " Hz, channels=" << m_publish_info.audio_channels;

    rtc::Configuration rtc_config;
    rtc_config.iceServers.push_back(rtc::IceServer("stun:stun.l.google.com:19302"));

    m_rtc_connection = std::make_shared<rtc::PeerConnection>(rtc_config);

    m_rtc_connection->onLocalCandidate([this](rtc::Candidate candidate) {
        std::string candidate_str = std::string(candidate);
        std::string mid = candidate.mid();
        LOG_INFO_STREAM << "Local ICE Candidate mid: " << mid << " candidate: " << candidate_str;
    });

    m_rtc_connection->onIceStateChange([this](rtc::PeerConnection::IceState state) {
        LOG_INFO_STREAM << "ICE State: " << static_cast<int>(state);
    });

    m_rtc_connection->onStateChange([this](rtc::PeerConnection::State state) {
        OnStateChange(state);
    });

    m_rtc_connection->onGatheringStateChange([this](rtc::PeerConnection::GatheringState state) {
        OnGatheringStateChange(state);
    });

    LOG_INFO("==========================================");
    return true;
}

bool RtcTransportBase::SetRemoteDescription(const std::string& sdp, const std::string& type)
{
    if (!m_rtc_connection)
    {
        return false;
    }

    LOG_INFO_STREAM << "set Remote description type: " << type;
    rtc::Description description(sdp, type);
    m_rtc_connection->setRemoteDescription(description);
    return true;
}

void RtcTransportBase::OnStateChange(rtc::PeerConnection::State state)
{
    LOG_INFO_STREAM << "PeerConnection State: " << static_cast<int>(state);

    ConnectionState new_state = ConnectionState::kDisconnected;

    switch (state)
    {
        case rtc::PeerConnection::State::New:
            new_state = ConnectionState::kDisconnected;
            break;
        case rtc::PeerConnection::State::Connecting:
            new_state = ConnectionState::kConnecting;
            break;
        case rtc::PeerConnection::State::Connected:
            LOG_INFO("  -> Connected");
            new_state = ConnectionState::kConnected;
            break;
        case rtc::PeerConnection::State::Disconnected:
            LOG_INFO("  -> Disconnected");
            new_state = ConnectionState::kDisconnected;
            break;
        case rtc::PeerConnection::State::Failed:
            LOG_INFO("  -> Failed");
            new_state = ConnectionState::kFailed;
            break;
        case rtc::PeerConnection::State::Closed:
            LOG_INFO("  -> Closed");
            new_state = ConnectionState::kFailed;
            break;
        default:
            new_state = ConnectionState::kDisconnected;
            break;
    }

    SetState(new_state);
}

void RtcTransportBase::OnGatheringStateChange(rtc::PeerConnection::GatheringState state)
{
    LOG_INFO_STREAM << "ICE Gathering State: " << static_cast<int>(state);

    if (state == rtc::PeerConnection::GatheringState::Complete)
    {
        auto description = m_rtc_connection->localDescription();
        if (!description)
        {
            LOG_INFO("Local description unavailable at gathering complete");
        }

        if (m_local_description_callback && description)
        {
            m_local_description_callback(description.value());
        }
    }
}

void RtcTransportBase::SetState(ConnectionState state)
{
    m_state = state;

    if (m_state_change_callback)
    {
        m_state_change_callback(state);
    }
}
}
