#ifndef __RTC_TRANSPORT_BASE_H__
#define __RTC_TRANSPORT_BASE_H__

#include "TransportDefine.h"
#include "rtc/peerconnection.hpp"
#include <memory>
#include <functional>
#include <string>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace rtc
{
    class PeerConnection;
    class Track;
}

namespace TRANSPORT
{
    constexpr uint8_t RTP_PAYLOAD_TYPE_H264 = 96;
    constexpr uint8_t RTP_PAYLOAD_TYPE_OPUS = 111;

    using OnVideoFrameCallback = std::function<void(const std::shared_ptr<I420Frame> &)>;
    using OnAudioFrameCallback = std::function<void(const std::shared_ptr<AudioFrame> &)>;
    using OnStateChangeCallback = std::function<void(ConnectionState)>;
    using OnLocalDescriptionCallback = std::function<void(const rtc::Description &)>;

    class RtcTransportBase
    {
    public:
        RtcTransportBase();
        virtual ~RtcTransportBase();

    public:
        bool Initialize(const PublishInfo &info);
        bool Open();
        virtual void Close() = 0;

        bool IsOpen() const { return m_state == ConnectionState::kConnected; }
        ConnectionState GetState() const { return m_state; }

        bool SetRemoteDescription(const std::string &sdp, const std::string &type);

        void SetStateChangeCallback(OnStateChangeCallback cb) { m_state_change_callback = cb; }
        void SetLocalDescriptionCallback(OnLocalDescriptionCallback cb) { m_local_description_callback = cb; }

        PublishInfo GetPublishInfo() const { return m_publish_info; }

        void SetUserId(const std::string &user_id) { m_local_peer_id = user_id; }

    protected:
        void SetState(ConnectionState state);

    private:
        void OnStateChange(rtc::PeerConnection::State state);
        void OnGatheringStateChange(rtc::PeerConnection::GatheringState state);

    protected:
        std::string m_local_peer_id;
        std::shared_ptr<rtc::PeerConnection> m_rtc_connection;
        std::atomic<ConnectionState> m_state{ConnectionState::kDisconnected};
        OnStateChangeCallback m_state_change_callback;
        OnLocalDescriptionCallback m_local_description_callback;
        PublishInfo m_publish_info;
    };

}

#endif // __RTC_TRANSPORT_BASE_H__
