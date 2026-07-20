#include "AudioCodecFactory.h"
#include "OpusCodec.h"

namespace TRANSPORT
{
namespace CODEC
{
    AudioCodecFactory& AudioCodecFactory::Instance()
    {
        static AudioCodecFactory instance;
        return instance;
    }

    std::unique_ptr<IAudioEncoder> AudioCodecFactory::CreateEncoder(AudioCodecType type)
    {
        switch (type)
        {
        case AudioCodecType::OPUS:
            return std::make_unique<OpusCodecEncoder>();
        default:
            return nullptr;
        }
    }

    std::unique_ptr<IAudioDecoder> AudioCodecFactory::CreateDecoder(AudioCodecType type)
    {
        switch (type)
        {
        case AudioCodecType::OPUS:
            return std::make_unique<OpusCodecDecoder>();
        default:
            return nullptr;
        }
    }

} // namespace CODEC
} // namespace TRANSPORT
