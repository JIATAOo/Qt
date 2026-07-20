#ifndef __TRANSPORT_ENGINE_H__
#define __TRANSPORT_ENGINE_H__

#include "rtc/description.hpp"
#include "ITransportEngine.h"
#include "WHIPWHEPClient.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace TRANSPORT
{
    class RtcPushTransport;
    class RtcPullTransport;

    class TransportEngine : public ITransportEngine
    {
    private:
        struct PullInfo
        {
            std::shared_ptr<RtcPullTransport> pull_rtc_transport;
            std::string pull_res_url;
        };

    public:
        TransportEngine();
        virtual ~TransportEngine();

        bool Initialize(const PublishInfo& publish_info) override;
        void Uninit() override;

        void SetTargetRoomInfo(const TransportTargetRoomInfo& config) override;
        TransportTargetRoomInfo GetTargetRoomInfo() const override;

        bool StartPublishVideo() override;
        bool StopPublishVideo() override;
        bool StartPublishAudio() override;
        bool StopPublishAudio() override;
        bool PushVideoFrame(const std::shared_ptr<I420Frame>& frame) override;
        bool PushAudioFrame(const std::shared_ptr<AudioFrame>& frame) override;
        bool IsPublishingVideo() override;
        bool IsPublishingAudio() override;
        
        bool SubscribeUserAV(const std::string& user_id) override;
        bool UnsubscribeUserAV(const std::string& user_id) override;
        bool IsUserSubscribedAV(const std::string& user_id) override;
        std::vector<std::string> GetSubscribedUsers() override;

        void RegisterVideoCallback(VideoDataCallback callback) override;
        void RegisterAudioCallback(AudioDataCallback callback) override;
        void RegisterConnectionStateCallback(ConnectionStateCallback callback) override;

        bool IsInitialized() const override
        {
            return m_signaling_client != nullptr;
        }

    private:
        void OnLocalDescription(const rtc::Description& description);
        void ClosePublish();
        void CloseAllPull();
        void ClosePushPull();
        bool IsConnected() const;
        ConnectionState GetState() const;

    private:
        std::unique_ptr<RtcPushTransport> m_push_transport;
        std::string m_publish_resource_url;
        std::unique_ptr<WHIPWHEPClient> m_signaling_client;

        std::mutex m_mutex;
        TransportTargetRoomInfo m_room_info;

        std::unordered_map<std::string, PullInfo> m_pull_info_vec;

        VideoDataCallback m_video_callback;
        AudioDataCallback m_audio_callback;
        ConnectionStateCallback m_connection_state_callback;
    };
}

#endif // __TRANSPORT_ENGINE_H__
