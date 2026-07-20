#include "VideoCodecFactory.h"
#include "H264Codec.h"

namespace TRANSPORT
{
namespace CODEC
{
    VideoCodecFactory& VideoCodecFactory::Instance()
    {
        static VideoCodecFactory instance;
        return instance;
    }

    std::unique_ptr<IVideoEncoder> VideoCodecFactory::CreateEncoder(VideoCodecType type)
    {
        switch (type)
        {
        case VideoCodecType::H264:
            return std::make_unique<H264Encoder>();
        default:
            return nullptr;
        }
    }

    std::unique_ptr<IVideoDecoder> VideoCodecFactory::CreateDecoder(VideoCodecType type)
    {
        switch (type)
        {
        case VideoCodecType::H264:
            return std::make_unique<H264Decoder>();
        default:
            return nullptr;
        }
    }

} // namespace CODEC
} // namespace TRANSPORT
