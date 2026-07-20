#include "TransportEngine.h"
#include "Logger.h"
#include "RtcPushTransport.h"
#include "RtcPullTransport.h"

namespace TRANSPORT
{
    std::unique_ptr<ITransportEngine> ITransportEngine::Create()
    {
        return std::make_unique<TransportEngine>();
    }

    TransportEngine::TransportEngine()
        : m_signaling_client(std::make_unique<WHIPWHEPClient>())
    {
        rtcPreload();
        m_push_transport = std::make_unique<RtcPushTransport>();
    }

    TransportEngine::~TransportEngine()
    {
        if (m_push_transport)
        {
            Uninit();
        }
    }

    bool TransportEngine::Initialize(const PublishInfo &publish_info)
    {
        if (!m_push_transport->Initialize(publish_info))
        {
            return false;
        }

        m_push_transport->SetLocalDescriptionCallback(
            [this](const rtc::Description &description)
            {
                OnLocalDescription(description);
            });

        m_push_transport->SetStateChangeCallback(
            [this](ConnectionState state)
            {
                if (m_connection_state_callback)
                {
                    m_connection_state_callback(state);
                }
            });

        return true;
    }

    void TransportEngine::Uninit()
    {
        RegisterVideoCallback(nullptr);
        RegisterAudioCallback(nullptr);
        RegisterConnectionStateCallback(nullptr);

        ClosePushPull();

        m_push_transport.reset();
        m_signaling_client.reset();
        rtcCleanup();
    }

    void TransportEngine::SetTargetRoomInfo(const TransportTargetRoomInfo &config)
    {
        m_room_info = config;
    }

    TransportTargetRoomInfo TransportEngine::GetTargetRoomInfo() const
    {
        return m_room_info;
    }

    void TransportEngine::ClosePushPull()
    {
        ClosePublish();
        CloseAllPull();
    }

    ConnectionState TransportEngine::GetState() const
    {
        if (!m_push_transport)
        {
            return ConnectionState::kDisconnected;
        }
        return m_push_transport->GetState();
    }

    bool TransportEngine::StartPublishVideo()
    {
        if (!m_push_transport) return false;

        if (!m_push_transport->IsOpen())
        {
            if (!m_push_transport->Open()) return false;
        }

        return m_push_transport->StartPublishVideo(m_room_info.local_user_id);
    }

    bool TransportEngine::StopPublishVideo()
    {
        if (!m_push_transport) return true;
        m_push_transport->StopPublishVideo();
        if (!m_push_transport->IsPublishingAudio()) ClosePublish();
        return true;
    }

    bool TransportEngine::StartPublishAudio()
    {
        if (!m_push_transport) return false;

        if (!m_push_transport->IsOpen())
        {
            if (!m_push_transport->Open()) return false;
        }

        return m_push_transport->StartPublishAudio(m_room_info.local_user_id);
    }

    bool TransportEngine::StopPublishAudio()
    {
        if (!m_push_transport) return true;
        m_push_transport->StopPublishAudio();
        if (!m_push_transport->IsPublishingVideo()) ClosePublish();
        return true;
    }

    void TransportEngine::ClosePublish()
    {
        if (!m_publish_resource_url.empty() && m_signaling_client)
        {
            m_signaling_client->Unpublish(m_publish_resource_url, m_room_info.push_server_url);
            m_publish_resource_url.clear();
        }

        if (m_push_transport)
        {
            m_push_transport->Close();
        }
    }

    void TransportEngine::CloseAllPull()
    {
        for (const auto &[user_id, sub] : m_pull_info_vec)
        {
            if (!sub.pull_res_url.empty() && m_signaling_client)
            {
                m_signaling_client->Unsubscribe(sub.pull_res_url, m_room_info.pull_server_url);
            }

            if (sub.pull_rtc_transport)
            {
                sub.pull_rtc_transport->Close();
            }
        }
        m_pull_info_vec.clear();
    }

    bool TransportEngine::PushVideoFrame(const std::shared_ptr<I420Frame> &frame)
    {
        if (!m_push_transport || !m_push_transport->IsOpen()) return false;
        return m_push_transport->PushVideoFrame(frame);
    }

    bool TransportEngine::PushAudioFrame(const std::shared_ptr<AudioFrame> &frame)
    {
        if (!m_push_transport || !m_push_transport->IsOpen()) return false;
        return m_push_transport->PushAudioFrame(frame);
    }

    bool TransportEngine::SubscribeUserAV(const std::string &user_id)
    {
        if (!m_signaling_client) return false;

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_pull_info_vec.find(user_id);
        if (it != m_pull_info_vec.end()) return true;

        LOG_INFO_STREAM << "SubscribeUserAV user_id: " << user_id
            << " Pull server URL: " << m_room_info.pull_server_url
            << " Room ID: " << m_room_info.room_id;

        auto transport = std::make_shared<RtcPullTransport>();
        transport->Initialize(m_push_transport->GetPublishInfo());
        transport->SetUserId(user_id);

        PullInfo subscribe_info;
        subscribe_info.pull_rtc_transport = transport;
        m_pull_info_vec[user_id] = subscribe_info;

        transport->SetLocalDescriptionCallback(
            [this, user_id](const rtc::Description &description)
            {
                SubscribeConfig whep_config;
                whep_config.endpoint_url = m_room_info.pull_server_url;
                whep_config.room_id = m_room_info.room_id;
                whep_config.remote_user_id = user_id;

                auto sdp_offer = description.generateSdp();
                if (sdp_offer.empty()) return;

                auto response = m_signaling_client->Subscribe(whep_config, sdp_offer);

                auto it = m_pull_info_vec.find(user_id);
                if (it == m_pull_info_vec.end()) return;

                auto &subscribe = it->second;
                if (!response.has_value())
                {
                    subscribe.pull_rtc_transport->Close();
                    return;
                }

                auto whip_response = response.value();
                auto sdp = whip_response.sdp;
                if (!sdp.empty())
                {
                    subscribe.pull_rtc_transport->SetRemoteDescription(sdp, "answer");
                }

                subscribe.pull_res_url = whip_response.resource_url;
            });

        transport->SetVideoFrameCallback(
            [this, user_id](const std::shared_ptr<I420Frame> &frame)
            {
                if (m_video_callback) m_video_callback(user_id, frame);
            });

        transport->SetAudioFrameCallback(
            [this, user_id](const std::shared_ptr<AudioFrame> &frame)
            {
                if (m_audio_callback) m_audio_callback(user_id, frame);
            });

        if (!transport->Open()) { m_pull_info_vec.erase(user_id); return false; }
        if (!transport->SubscribeAudioVideo()) { m_pull_info_vec.erase(user_id); return false; }

        return true;
    }

    bool TransportEngine::UnsubscribeUserAV(const std::string &user_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_pull_info_vec.find(user_id);
        if (it == m_pull_info_vec.end()) return false;

        auto &subscribe = it->second;

        if (!subscribe.pull_res_url.empty() && m_signaling_client)
        {
            m_signaling_client->Unsubscribe(subscribe.pull_res_url, m_room_info.pull_server_url);
        }

        if (subscribe.pull_rtc_transport)
        {
            subscribe.pull_rtc_transport->Close();
        }

        m_pull_info_vec.erase(it);
        return true;
    }

    bool TransportEngine::IsUserSubscribedAV(const std::string &user_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_pull_info_vec.find(user_id) != m_pull_info_vec.end();
    }

    void TransportEngine::OnLocalDescription(const rtc::Description &description)
    {
        if (description.type() == rtc::Description::Type::Offer)
        {
            PublishConfig whip_config;
            whip_config.endpoint_url = m_room_info.push_server_url;
            whip_config.room_id = m_room_info.room_id;
            whip_config.local_user_id = m_room_info.local_user_id;

            rtc::Description des = description;
            std::string sdp = des.generateSdp();

            auto optional_response = m_signaling_client->Publish(whip_config, sdp);

            if (!optional_response.has_value()) return;

            auto response = optional_response.value();
            if (!response.sdp.empty())
            {
                LOG_INFO("========== Remote Answer SDP ==========");
                LOG_INFO_STREAM << response.sdp;
                LOG_INFO("=======================================");
                m_push_transport->SetRemoteDescription(response.sdp, "answer");
            }

            if (!response.resource_url.empty())
            {
                m_publish_resource_url = response.resource_url;
            }

            LOG_INFO("WHIP Publish successful!");
        }
    }

    void TransportEngine::RegisterVideoCallback(VideoDataCallback callback)
    {
        if (!callback) return;
        m_video_callback = callback;
    }

    void TransportEngine::RegisterAudioCallback(AudioDataCallback callback)
    {
        if (!callback) return;
        m_audio_callback = callback;
    }

    void TransportEngine::RegisterConnectionStateCallback(ConnectionStateCallback callback)
    {
        if (!callback) return;
        m_connection_state_callback = callback;
    }

    bool TransportEngine::IsPublishingVideo()
    {
        if (!m_push_transport || !m_push_transport->IsOpen()) return false;
        return m_push_transport->IsPublishingVideo();
    }

    bool TransportEngine::IsPublishingAudio()
    {
        if (!m_push_transport || !m_push_transport->IsOpen()) return false;
        return m_push_transport->IsPublishingAudio();
    }

    std::vector<std::string> TransportEngine::GetSubscribedUsers()
    {
        std::vector<std::string> users;
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto &[user_id, sub] : m_pull_info_vec)
        {
            users.push_back(user_id);
        }
        return users;
    }

}
