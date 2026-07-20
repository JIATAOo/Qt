#ifndef __ITRANSPORTENGINE_H__
#define __ITRANSPORTENGINE_H__

#include "TransportDefine.h"
#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace TRANSPORT
{
    class ITransportEngine
    {
    public:
        virtual ~ITransportEngine() = default;

        static std::unique_ptr<ITransportEngine> Create();

        virtual bool Initialize(const PublishInfo& config) = 0;
        virtual void Uninit() = 0;

        virtual void SetTargetRoomInfo(const TransportTargetRoomInfo& info) = 0;
        virtual TransportTargetRoomInfo GetTargetRoomInfo() const = 0;

        virtual bool StartPublishVideo() = 0;
        virtual bool StopPublishVideo() = 0;
        virtual bool StartPublishAudio() = 0;
        virtual bool StopPublishAudio() = 0;
        virtual bool PushVideoFrame(const std::shared_ptr<I420Frame>& frame) = 0;
        virtual bool PushAudioFrame(const std::shared_ptr<AudioFrame>& frame) = 0;
        virtual bool IsPublishingVideo() = 0;
        virtual bool IsPublishingAudio() = 0;

        virtual bool SubscribeUserAV(const std::string& user_id) = 0;
        virtual bool UnsubscribeUserAV(const std::string& user_id) = 0;
        virtual bool IsUserSubscribedAV(const std::string& user_id) = 0;
        virtual std::vector<std::string> GetSubscribedUsers() = 0;

        using VideoDataCallback = std::function<void(const std::string&, const std::shared_ptr<I420Frame>&)>;
        using AudioDataCallback = std::function<void(const std::string&, const std::shared_ptr<AudioFrame>&)>;
        using ConnectionStateCallback = std::function<void(ConnectionState)>;

        virtual void RegisterVideoCallback(VideoDataCallback) = 0;
        virtual void RegisterAudioCallback(AudioDataCallback) = 0;
        virtual void RegisterConnectionStateCallback(ConnectionStateCallback) = 0;

        virtual bool IsInitialized() const = 0;
    };
}

#endif
