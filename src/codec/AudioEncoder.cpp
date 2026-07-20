#include "AudioEncoder.h"

extern "C" {
#include "opus/opus.h"
}

#include <cstring>
#include <vector>
#include <algorithm>

namespace Codec {

AudioEncoder::AudioEncoder() = default;

AudioEncoder::~AudioEncoder()
{
    if (encoder_) {
        opus_encoder_destroy(encoder_);
        encoder_ = nullptr;
    }
}

bool AudioEncoder::init(int sampleRate, int channels, int bitrate)
{
    if (initialized_) return true;

    sampleRate_ = sampleRate;
    channels_   = channels;

    // Opus 固定使用 48000 Hz
    const int opusSampleRate = 48000;

    int error = 0;
    encoder_ = opus_encoder_create(opusSampleRate, channels, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK || !encoder_) {
        return false;
    }

    // 配置编码参数
    opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(bitrate));
    opus_encoder_ctl(encoder_, OPUS_SET_VBR(1));          // 可变码率
    opus_encoder_ctl(encoder_, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(encoder_, OPUS_SET_COMPLEXITY(5));   // 中等复杂度
    opus_encoder_ctl(encoder_, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(encoder_, OPUS_SET_DTX(0));          // 不启用 DTX
    opus_encoder_ctl(encoder_, OPUS_SET_INBAND_FEC(1));   // 带内 FEC
    opus_encoder_ctl(encoder_, OPUS_SET_PACKET_LOSS_PERC(10));

    // 帧大小：20ms = 960 samples at 48kHz
    frameSize_     = 960;
    maxPacketSize_ = 4000;

    initialized_ = true;
    return true;
}

EncodedAudioFrame AudioEncoder::encode(const float *pcmData, int samples,
                                        int channels, int64_t timestampUs)
{
    EncodedAudioFrame result;
    if (!initialized_ || !encoder_) return result;

    const int opusSampleRate = 48000;
    const int targetSamples = frameSize_;

    // 如果输入采样率不是 48000，做简单重采样
    std::vector<float> resampled;
    const float *inputData = pcmData;
    int inputSamples = samples;

    if (sampleRate_ != opusSampleRate) {
        // 简单线性重采样
        double ratio = (double)opusSampleRate / sampleRate_;
        int outSamples = (int)(samples * ratio);
        resampled.resize(outSamples * channels);

        for (int ch = 0; ch < channels; ++ch) {
            for (int i = 0; i < outSamples; ++i) {
                double srcIdx = i / ratio;
                int idx0 = (int)srcIdx;
                int idx1 = std::min(idx0 + 1, samples - 1);
                double frac = srcIdx - idx0;

                resampled[i * channels + ch] =
                    (float)(pcmData[idx0 * channels + ch] * (1.0 - frac) +
                            pcmData[idx1 * channels + ch] * frac);
            }
        }
        inputData    = resampled.data();
        inputSamples = outSamples;
    }

    // 如果声道数不匹配，做声道转换
    std::vector<float> channelConverted;
    if (channels_ != channels) {
        int outSamples = inputSamples;
        channelConverted.resize(outSamples * channels_);

        if (channels_ == 1 && channels == 2) {
            // 立体声 -> 单声道：取平均
            for (int i = 0; i < outSamples; ++i) {
                channelConverted[i] = (inputData[i * 2] + inputData[i * 2 + 1]) * 0.5f;
            }
        } else if (channels_ == 2 && channels == 1) {
            // 单声道 -> 立体声：复制
            for (int i = 0; i < outSamples; ++i) {
                channelConverted[i * 2]     = inputData[i];
                channelConverted[i * 2 + 1] = inputData[i];
            }
        }
        inputData    = channelConverted.data();
    }

    // 按 20ms 帧编码，不足补齐静音
    std::vector<float> frameBuf(targetSamples * channels_);
    std::vector<uint8_t> encBuf(maxPacketSize_);

    int offset = 0;
    while (offset + targetSamples <= inputSamples) {
        // 复制一帧数据
        std::memcpy(frameBuf.data(), inputData + offset * channels_,
                    targetSamples * channels_ * sizeof(float));
        offset += targetSamples;

        int encoded = opus_encode_float(encoder_, frameBuf.data(), targetSamples,
                                         encBuf.data(), maxPacketSize_);
        if (encoded > 0) {
            result.data = std::shared_ptr<uint8_t[]>(new uint8_t[encoded]);
            result.size = encoded;
            result.timestampUs = timestampUs;
            std::memcpy(result.data.get(), encBuf.data(), encoded);
            return result;  // 返回第一个编码帧
        }
    }

    return result;
}

} // namespace Codec
