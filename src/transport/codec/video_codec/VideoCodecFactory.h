#ifndef __VIDEO_CODEC_FACTORY_H__
#define __VIDEO_CODEC_FACTORY_H__

#include "VideoCodecInterface.h"
#include <memory>

namespace TRANSPORT
{
namespace CODEC
{
    class VideoCodecFactory
    {
    public:
        static VideoCodecFactory& Instance();

        VideoCodecFactory(const VideoCodecFactory&) = delete;
        VideoCodecFactory& operator=(const VideoCodecFactory&) = delete;
        VideoCodecFactory(VideoCodecFactory&&) = delete;
        VideoCodecFactory& operator=(VideoCodecFactory&&) = delete;

        std::unique_ptr<IVideoEncoder> CreateEncoder(VideoCodecType type);
        std::unique_ptr<IVideoDecoder> CreateDecoder(VideoCodecType type);

    private:
        VideoCodecFactory() = default;
        ~VideoCodecFactory() = default;
    };

} // namespace CODEC
} // namespace TRANSPORT

#endif // __VIDEO_CODEC_FACTORY_H__
