#ifndef __VIDEO_CODEC_INTERFACE_H__
#define __VIDEO_CODEC_INTERFACE_H__

#include <vector>
#include <memory>
#include <cstdint>
#include <string>

namespace TRANSPORT
{
namespace CODEC
{
    enum class VideoCodecType
    {
        H264 = 0
    };

    enum class VideoFormat
    {
        I420,
    };

    struct VideoCodecConfig
    {
        uint32_t width = 1920;
        uint32_t height = 1080;
        uint32_t framerate = 30;
        uint32_t target_bitrate_kbps = 2000;
        uint32_t max_bitrate_kbps = 3000;
        uint32_t min_bitrate_kbps = 500;
        uint32_t keyframe_interval = 60;
        uint32_t threads = 0;
        bool enable_adaptive_bitrate = true;
        bool enable_frame_dropping = true;
        std::vector<std::pair<std::string, std::string>> custom_params;
    };

    struct EncodedVideoFrame
    {
        std::vector<uint8_t> data;
        uint64_t timestamp_us;
        bool is_skip = false;
        uint32_t width;
        uint32_t height;
        VideoCodecType codec_type;
        uint32_t temporal_idx = 0;
        uint32_t spatial_idx = 0;
    };

    struct RawVideoFrame
    {
        std::shared_ptr<uint8_t[]> data[3];
        uint32_t width = 0;
        uint32_t height = 0;
        uint64_t timestamp_us = 0;
        VideoFormat format = VideoFormat::I420;
        uint32_t stride[3] = {0, 0, 0};

        RawVideoFrame() = default;
        RawVideoFrame(const RawVideoFrame&) = default;
        RawVideoFrame& operator=(const RawVideoFrame&) = default;
        RawVideoFrame(RawVideoFrame&&) noexcept = default;
        RawVideoFrame& operator=(RawVideoFrame&&) noexcept = default;

        bool IsValid() const {
            return data[0] != nullptr && data[1] != nullptr && data[2] != nullptr &&
                   width > 0 && height > 0;
        }
    };

    class IVideoEncoder
    {
    public:
        virtual ~IVideoEncoder() = default;
        virtual bool Initialize(const VideoCodecConfig& config) = 0;
        virtual void Release() = 0;
        virtual bool Encode(const RawVideoFrame& input, EncodedVideoFrame& output) = 0;
        virtual bool SetBitrate(uint32_t bitrate_kbps) = 0;
        virtual bool SetFramerate(uint32_t fps) = 0;
        virtual bool RequestKeyframe() = 0;
        virtual VideoCodecType GetCodecType() const = 0;
    };

    class IVideoDecoder
    {
    public:
        virtual ~IVideoDecoder() = default;
        virtual bool Initialize(const VideoCodecConfig& config) = 0;
        virtual void Release() = 0;
        virtual bool Decode(const EncodedVideoFrame& input, RawVideoFrame& output) = 0;
        virtual VideoCodecType GetCodecType() const = 0;
        virtual bool NeedsKeyframe() const { return false; }
        virtual void ClearKeyframeRequest() {}
    };

} // namespace CODEC
} // namespace TRANSPORT

#endif // __VIDEO_CODEC_INTERFACE_H__
