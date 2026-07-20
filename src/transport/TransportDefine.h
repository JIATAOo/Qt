#ifndef __TRANSPORT_DEFINE_H__
#define __TRANSPORT_DEFINE_H__

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace TRANSPORT
{
    struct I420Frame
    {
        int width = 0;
        int height = 0;
        int format = 0;
        int64_t timestamp_us = 0;
        std::shared_ptr<uint8_t[]> data[3];
    };

    struct AudioFrame
    {
        int samples = 0;
        int sample_rate = 0;
        int channels = 0;
        int format = 0;
        int64_t timestamp_us = 0;
        std::shared_ptr<uint8_t[]> data;
    };

    enum class ConnectionState
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting,
        kFailed
    };

    struct PublishInfo
    {
        int video_width = 0;
        int video_height = 0;
        int video_fps = 0;
        int audio_sample_rate = 0;
        int audio_channels = 0;
    };

    struct TransportTargetRoomInfo
    {
        std::string push_server_url;
        std::string pull_server_url;
        std::string room_id;
        std::string local_user_id;
    };
}

#endif
