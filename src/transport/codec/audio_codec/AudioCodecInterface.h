#ifndef __AUDIO_CODEC_INTERFACE_H__
#define __AUDIO_CODEC_INTERFACE_H__

#include <vector>
#include <memory>
#include <cstdint>
#include <string>

namespace TRANSPORT
{
namespace CODEC
{
    enum class AudioCodecType
    {
        OPUS= 0,
    };

    enum class AudioSampleFormat
    {
        PCM_FLOAT,
    };

    struct AudioCodecConfig
    {
        uint32_t sample_rate = 48000;
        uint32_t channels = 2;
        uint32_t bitrate_kbps = 64;
        AudioSampleFormat format = AudioSampleFormat::PCM_FLOAT;
        uint32_t frame_size_ms = 20;
        uint32_t complexity = 5;
        bool enable_vbr = true;
        bool enable_dtx = false;
        bool enable_fec = true;
        bool enable_plc = true;
        std::vector<std::pair<std::string, std::string>> custom_params;
    };

    struct EncodedAudioFrame
    {
        std::vector<uint8_t> data;
        uint64_t timestamp_us = 0;
        uint32_t sample_count = 0;
        AudioCodecType codec_type = AudioCodecType::OPUS;
        bool has_fec = false;
        std::vector<uint8_t> fec_data;
    };

    struct RawAudioFrame
    {
        std::shared_ptr<uint8_t[]> data;
        uint32_t sample_rate = 0;
        uint32_t channels = 0;
        uint32_t sample_per_channel = 0;
        uint64_t timestamp_us = 0;
        AudioSampleFormat format = AudioSampleFormat::PCM_FLOAT;

        RawAudioFrame() = default;
        RawAudioFrame(const RawAudioFrame&) = default;
        RawAudioFrame& operator=(const RawAudioFrame&) = default;
        RawAudioFrame(RawAudioFrame&&) noexcept = default;
        RawAudioFrame& operator=(RawAudioFrame&&) noexcept = default;

        bool IsValid() const {
            return data != nullptr && sample_per_channel > 0 && channels > 0 && sample_rate > 0;
        }
    };

    class IAudioEncoder
    {
    public:
        virtual ~IAudioEncoder() = default;
        virtual bool Initialize(const AudioCodecConfig& config) = 0;
        virtual void Release() = 0;
        virtual bool PushInput(const RawAudioFrame& input) = 0;
        virtual bool PullEncoded(EncodedAudioFrame& output) = 0;
        virtual size_t GetPendingFrameCount() const = 0;
        virtual bool Flush() = 0;
        virtual bool SetBitrate(uint32_t bitrate_kbps) = 0;
        virtual bool SetComplexity(uint32_t complexity) = 0;
        virtual bool SetVBR(bool enable) = 0;
        virtual bool SetDTX(bool enable) = 0;
        virtual bool SetFEC(bool enable) = 0;
        virtual AudioCodecType GetCodecType() const = 0;
        virtual uint32_t GetFrameSizeSamples() const = 0;
        virtual bool SetParameter(const std::string& key, const std::string& value) { return false; }
    };

    class IAudioDecoder
    {
    public:
        virtual ~IAudioDecoder() = default;
        virtual bool Initialize(const AudioCodecConfig& config) = 0;
        virtual void Release() = 0;
        virtual bool PushInput(const EncodedAudioFrame& input) = 0;
        virtual bool PullDecoded(RawAudioFrame& output) = 0;
        virtual size_t GetPendingFrameCount() const = 0;
        virtual bool Flush() = 0;
        virtual bool ConcealLostPacket(RawAudioFrame& output) = 0;
        virtual AudioCodecType GetCodecType() const = 0;
        virtual uint32_t GetFrameSizeSamples() const = 0;
        virtual bool SetParameter(const std::string& key, const std::string& value) { return false; }
    };

} // namespace CODEC
} // namespace TRANSPORT

#endif // __AUDIO_CODEC_INTERFACE_H__
