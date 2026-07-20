#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

#include <cstdint>
#include <memory>

struct OpusEncoder;

namespace Codec {

struct EncodedAudioFrame {
    std::shared_ptr<uint8_t[]> data;
    int size = 0;
    int64_t timestampUs = 0;

    bool valid() const { return data != nullptr && size > 0; }
};

class AudioEncoder {
public:
    AudioEncoder();
    ~AudioEncoder();

    // 初始化编码器
    // sampleRate: 输入采样率 (会自动重采样到 48000)
    // channels: 声道数 (1 或 2)
    // bitrate: 目标码率 bps
    bool init(int sampleRate, int channels, int bitrate = 64000);

    // 编码 PCM float 交叠音频帧，返回 Opus 编码数据
    EncodedAudioFrame encode(const float *pcmData, int samples, int channels,
                             int64_t timestampUs);

    bool ready() const { return initialized_; }

private:
    OpusEncoder *encoder_ = nullptr;
    bool initialized_ = false;
    int sampleRate_ = 0;
    int channels_ = 0;
    int frameSize_ = 960;  // 20ms at 48kHz
    int maxPacketSize_ = 4000;

    // 简单线性插值重采样（输入任意采样率 -> 48000）
    // 对于实时音频应用，这种简单方法已足够
};

} // namespace Codec

#endif // __AUDIO_ENCODER_H__
