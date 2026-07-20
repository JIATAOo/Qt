#ifndef __OPUS_CODEC_H__
#define __OPUS_CODEC_H__

#include "AudioCodecInterface.h"
#include <memory>
#include <vector>
#include <queue>
#include <mutex>

namespace TRANSPORT {
namespace CODEC {

class OpusCodecEncoder : public IAudioEncoder
{
public:
    OpusCodecEncoder();
    ~OpusCodecEncoder() override;

    bool Initialize(const AudioCodecConfig& config) override;
    void Release() override;
    bool PushInput(const RawAudioFrame& input) override;
    bool PullEncoded(EncodedAudioFrame& output) override;
    size_t GetPendingFrameCount() const override;
    bool Flush() override;
    bool SetBitrate(uint32_t bitrate_kbps) override;
    bool SetComplexity(uint32_t complexity) override;
    bool SetVBR(bool enable) override;
    bool SetDTX(bool enable) override;
    bool SetFEC(bool enable) override;
    AudioCodecType GetCodecType() const override { return AudioCodecType::OPUS; }
    uint32_t GetFrameSizeSamples() const override;

private:
    void ReleaseInternal();
    bool InitResampler();
    std::vector<float> ResampleAudio(const float* input, size_t input_frames);
    void ProcessBuffer(uint64_t timestamp_us);
    void EncodeFrame(uint64_t timestamp_us);

    struct EncoderContext;
    std::unique_ptr<EncoderContext> m_context;
    AudioCodecConfig m_config;
    bool m_initialized;
    mutable std::mutex m_mutex;
};

class OpusCodecDecoder : public IAudioDecoder
{
public:
    OpusCodecDecoder();
    ~OpusCodecDecoder() override;

    bool Initialize(const AudioCodecConfig& config) override;
    void Release() override;
    bool PushInput(const EncodedAudioFrame& input) override;
    bool PullDecoded(RawAudioFrame& output) override;
    size_t GetPendingFrameCount() const override;
    bool Flush() override;
    bool ConcealLostPacket(RawAudioFrame& output) override;
    AudioCodecType GetCodecType() const override { return AudioCodecType::OPUS; }
    uint32_t GetFrameSizeSamples() const override;

private:
    void ReleaseInternal();
    bool InitResampler();
    std::vector<float> ResampleAudio(const float* input, size_t input_frames);
    void ProcessBuffer(uint64_t timestamp_us);
    void CreateOutputFrame(uint64_t timestamp_us);

    struct DecoderContext;
    std::unique_ptr<DecoderContext> m_context;
    AudioCodecConfig m_config;
    bool m_initialized;
    mutable std::mutex m_mutex;
};

} // namespace CODEC
} // namespace TRANSPORT

#endif // __OPUS_CODEC_H__
