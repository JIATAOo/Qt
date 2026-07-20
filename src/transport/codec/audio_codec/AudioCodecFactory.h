#ifndef __AUDIO_CODEC_FACTORY_H__
#define __AUDIO_CODEC_FACTORY_H__

#include "AudioCodecInterface.h"
#include <memory>

namespace TRANSPORT
{
namespace CODEC
{
    class AudioCodecFactory
    {
    public:
        static AudioCodecFactory& Instance();

        AudioCodecFactory(const AudioCodecFactory&) = delete;
        AudioCodecFactory& operator=(const AudioCodecFactory&) = delete;
        AudioCodecFactory(AudioCodecFactory&&) = delete;
        AudioCodecFactory& operator=(AudioCodecFactory&&) = delete;

        std::unique_ptr<IAudioEncoder> CreateEncoder(AudioCodecType type);
        std::unique_ptr<IAudioDecoder> CreateDecoder(AudioCodecType type);

    private:
        AudioCodecFactory() = default;
        ~AudioCodecFactory() = default;
    };

} // namespace CODEC
} // namespace TRANSPORT

#endif // __AUDIO_CODEC_FACTORY_H__
