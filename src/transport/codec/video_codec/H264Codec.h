#ifndef __H264_CODEC_H__
#define __H264_CODEC_H__

#include "VideoCodecInterface.h"
#include <memory>
#include <vector>
#include <mutex>

namespace TRANSPORT {
namespace CODEC {

class H264Encoder : public IVideoEncoder
{
public:
    H264Encoder();
    ~H264Encoder() override;

    bool Initialize(const VideoCodecConfig& config) override;
    void Release() override;
    bool Encode(const RawVideoFrame& input, EncodedVideoFrame& output) override;
    bool SetBitrate(uint32_t bitrate_kbps) override;
    bool SetFramerate(uint32_t fps) override;
    bool RequestKeyframe() override;
    VideoCodecType GetCodecType() const override { return VideoCodecType::H264; }

private:
    void ReleaseInternal();

private:
    struct EncoderContext;
    std::unique_ptr<EncoderContext> m_context;
    VideoCodecConfig m_config;
    bool m_initialized;
    bool m_request_keyframe;
    mutable std::mutex m_mutex;
};

class H264Decoder : public IVideoDecoder
{
public:
    H264Decoder();
    ~H264Decoder() override;

    bool Initialize(const VideoCodecConfig& config) override;
    void Release() override;
    bool Decode(const EncodedVideoFrame& input, RawVideoFrame& output) override;
    VideoCodecType GetCodecType() const override { return VideoCodecType::H264; }
    bool NeedsKeyframe() const override;
    void ClearKeyframeRequest() override;

private:
    void ReleaseInternal();

private:
    struct DecoderContext;
    std::unique_ptr<DecoderContext> m_context;
    VideoCodecConfig m_config;
    bool m_initialized;
    bool m_request_keyframe;
    mutable std::mutex m_mutex;
};

} // namespace CODEC
} // namespace TRANSPORT

#endif // __H264_CODEC_H__
