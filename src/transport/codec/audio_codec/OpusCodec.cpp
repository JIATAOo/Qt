#include "OpusCodec.h"
#include "Logger.h"
#include <cstring>
#include <algorithm>

extern "C" {
#include "opus/opus.h"
#include "samplerate.h"
}

namespace TRANSPORT {
namespace CODEC {

static constexpr uint32_t sc_opus_default_sample_rate = 48000;
static constexpr uint32_t sc_opus_default_codec_frame_size = 960;

struct OpusCodecEncoder::EncoderContext {
    OpusEncoder* encoder = nullptr;
    SRC_STATE* resampler = nullptr;
    uint32_t input_sample_rate = 48000;
    uint32_t channels = 2;
    uint32_t frame_size = 960;
    uint32_t bitrate_kbps = 64;
    bool need_resample = false;
    std::vector<float> sample_buffer;
    std::vector<float> input_buffer;
    std::vector<uint8_t> output_buffer;
    std::queue<EncodedAudioFrame> output_queue;
    uint64_t last_timestamp_us = 0;
};

struct OpusCodecDecoder::DecoderContext {
    OpusDecoder* decoder = nullptr;
    SRC_STATE* resampler = nullptr;
    uint32_t output_sample_rate = 48000;
    uint32_t channels = 2;
    uint32_t output_frame_size = 1024;
    bool need_resample = false;
    std::vector<float> sample_buffer;
    std::vector<float> output_buffer;
    std::queue<RawAudioFrame> output_queue;
    uint64_t last_timestamp_us = 0;
};

OpusCodecEncoder::OpusCodecEncoder()
    : m_context(std::make_unique<EncoderContext>())
    , m_initialized(false) {
}

OpusCodecEncoder::~OpusCodecEncoder() {
    Release();
}

bool OpusCodecEncoder::Initialize(const AudioCodecConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) {
        ReleaseInternal();
    }

    m_config = config;

    m_context->input_sample_rate = config.sample_rate;
    m_context->channels = config.channels;
    m_context->frame_size = config.frame_size_ms * config.sample_rate / 1000;
    m_context->bitrate_kbps = config.bitrate_kbps;
    m_context->need_resample = (config.sample_rate != sc_opus_default_sample_rate);
    
    LOG_INFO_STREAM << "[Opus Encoder] input_rate=" << config.sample_rate 
                    << ", channels=" << config.channels
                    << ", bitrate=" << config.bitrate_kbps << "kbps";
    
    int error = 0;
    m_context->encoder = opus_encoder_create(
        sc_opus_default_sample_rate, config.channels,
        OPUS_APPLICATION_AUDIO, &error);

    if (error != OPUS_OK || !m_context->encoder) {
        LOG_ERROR_STREAM << "[Opus Encoder] FAILED to create encoder: error=" << error;
        return false;
    }

    opus_encoder_ctl(m_context->encoder, OPUS_SET_BITRATE(config.bitrate_kbps * 1000));
    opus_encoder_ctl(m_context->encoder, OPUS_SET_VBR(config.enable_vbr ? 1 : 0));
    opus_encoder_ctl(m_context->encoder, OPUS_SET_DTX(config.enable_dtx ? 1 : 0));
    opus_encoder_ctl(m_context->encoder, OPUS_SET_INBAND_FEC(config.enable_fec ? 1 : 0));
    opus_encoder_ctl(m_context->encoder, OPUS_SET_COMPLEXITY(config.complexity));

    m_context->sample_buffer.reserve(m_context->frame_size * m_context->channels * 4);
    m_context->input_buffer.resize(m_context->frame_size * m_context->channels);
    m_context->output_buffer.resize(4000);

    if (m_context->need_resample) {
        if (!InitResampler()) {
            ReleaseInternal();
            return false;
        }
    }

    m_initialized = true;
    LOG_INFO_STREAM << "[Opus Encoder] SUCCESS: frame_size=" << m_context->frame_size;
    return true;
}

bool OpusCodecEncoder::InitResampler() {
    int error = 0;
    m_context->resampler = src_new(SRC_SINC_FASTEST, m_context->channels, &error);
    if (error != 0 || !m_context->resampler) {
        LOG_ERROR_STREAM << "[Opus Encoder] FAILED to create resampler: error=" << error;
        return false;
    }
    return true;
}

void OpusCodecEncoder::ReleaseInternal() {
    if (m_context->resampler) {
        src_delete(m_context->resampler);
        m_context->resampler = nullptr;
    }
    if (m_context->encoder) {
        opus_encoder_destroy(m_context->encoder);
        m_context->encoder = nullptr;
    }
    m_context->sample_buffer.clear();
    while (!m_context->output_queue.empty()) {
        m_context->output_queue.pop();
    }
    m_initialized = false;
}

void OpusCodecEncoder::Release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReleaseInternal();
}

std::vector<float> OpusCodecEncoder::ResampleAudio(const float* input, size_t input_frames) {
    if (!m_context->need_resample || !m_context->resampler) {
        return std::vector<float>(input, input + input_frames * m_context->channels);
    }
    
    double ratio = static_cast<double>(sc_opus_default_sample_rate) / static_cast<double>(m_context->input_sample_rate);
    size_t output_frames = static_cast<size_t>(input_frames * ratio + 0.5);
    std::vector<float> output(output_frames * m_context->channels);
    
    SRC_DATA src_data;
    src_data.data_in = input;
    src_data.input_frames = static_cast<long>(input_frames);
    src_data.data_out = output.data();
    src_data.output_frames = static_cast<long>(output_frames);
    src_data.src_ratio = ratio;
    src_data.end_of_input = 0;
    
    int error = src_process(m_context->resampler, &src_data);
    if (error != 0) {
        LOG_ERROR_STREAM << "[Opus Encoder] Resampling error: " << src_strerror(error);
        return {};
    }
    
    output.resize(src_data.output_frames_gen * m_context->channels);
    return output;
}

void OpusCodecEncoder::EncodeFrame(uint64_t timestamp_us) {
    size_t required_samples = m_context->frame_size * m_context->channels;
    if (m_context->sample_buffer.size() < required_samples) return;

    std::copy_n(m_context->sample_buffer.begin(), required_samples, 
                m_context->input_buffer.begin());
    
    m_context->sample_buffer.erase(
        m_context->sample_buffer.begin(),
        m_context->sample_buffer.begin() + required_samples);
    
    int encoded_bytes = opus_encode_float(
        m_context->encoder,
        m_context->input_buffer.data(),
        m_context->frame_size,
        m_context->output_buffer.data(),
        static_cast<opus_int32>(m_context->output_buffer.size()));
    
    if (encoded_bytes < 0) {
        LOG_ERROR_STREAM << "[Opus Encoder] FAILED: opus_encode_float returned " << encoded_bytes;
        return;
    }
    
    EncodedAudioFrame frame;
    frame.data.assign(m_context->output_buffer.begin(),
                      m_context->output_buffer.begin() + encoded_bytes);
    frame.timestamp_us = timestamp_us;
    frame.sample_count = m_context->frame_size;
    frame.codec_type = AudioCodecType::OPUS;
    frame.has_fec = m_config.enable_fec;
    
    m_context->output_queue.push(std::move(frame));
}

void OpusCodecEncoder::ProcessBuffer(uint64_t timestamp_us) {
    size_t required_samples = m_context->frame_size * m_context->channels;
    while (m_context->sample_buffer.size() >= required_samples) {
        EncodeFrame(timestamp_us);
    }
}

bool OpusCodecEncoder::PushInput(const RawAudioFrame& input) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->encoder) return false;
    if (!input.IsValid()) return false;

    const float* input_data = reinterpret_cast<const float*>(input.data.get());
    size_t input_frames = input.sample_per_channel;
    
    std::vector<float> process_data;
    const float* data_to_buffer = nullptr;
    size_t frames_to_buffer = 0;
    
    if (m_context->need_resample) {
        process_data = ResampleAudio(input_data, input_frames);
        if (process_data.empty()) return false;
        data_to_buffer = process_data.data();
        frames_to_buffer = process_data.size() / m_context->channels;
    } else {
        data_to_buffer = input_data;
        frames_to_buffer = input_frames;
    }
    
    for (size_t i = 0; i < frames_to_buffer * m_context->channels; ++i) {
        float clamped = std::max(-1.0f, std::min(1.0f, data_to_buffer[i]));
        m_context->sample_buffer.push_back(clamped);
    }
    
    ProcessBuffer(input.timestamp_us);
    m_context->last_timestamp_us = input.timestamp_us;
    return true;
}

bool OpusCodecEncoder::PullEncoded(EncodedAudioFrame& output) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || m_context->output_queue.empty()) return false;
    output = std::move(m_context->output_queue.front());
    m_context->output_queue.pop();
    return true;
}

size_t OpusCodecEncoder::GetPendingFrameCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_context->output_queue.size();
}

bool OpusCodecEncoder::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->encoder) return false;

    size_t required_samples = m_context->frame_size * m_context->channels;
    if (m_context->sample_buffer.size() > 0 && m_context->sample_buffer.size() < required_samples) {
        size_t padding = required_samples - m_context->sample_buffer.size();
        m_context->sample_buffer.resize(required_samples, 0.0f);
        LOG_DEBUG_STREAM << "[Opus Encoder] Flush: padding " << padding << " samples";
        EncodeFrame(m_context->last_timestamp_us);
    }
    return true;
}

bool OpusCodecEncoder::SetBitrate(uint32_t bitrate_kbps) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->encoder) return false;
    int result = opus_encoder_ctl(m_context->encoder, OPUS_SET_BITRATE(bitrate_kbps * 1000));
    if (result != OPUS_OK) return false;
    m_context->bitrate_kbps = bitrate_kbps;
    m_config.bitrate_kbps = bitrate_kbps;
    return true;
}

bool OpusCodecEncoder::SetComplexity(uint32_t complexity) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->encoder) return false;
    complexity = std::min(complexity, 10u);
    int result = opus_encoder_ctl(m_context->encoder, OPUS_SET_COMPLEXITY(complexity));
    if (result != OPUS_OK) return false;
    m_config.complexity = complexity;
    return true;
}

bool OpusCodecEncoder::SetVBR(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->encoder) return false;
    int result = opus_encoder_ctl(m_context->encoder, OPUS_SET_VBR(enable ? 1 : 0));
    if (result != OPUS_OK) return false;
    m_config.enable_vbr = enable;
    return true;
}

bool OpusCodecEncoder::SetDTX(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->encoder) return false;
    int result = opus_encoder_ctl(m_context->encoder, OPUS_SET_DTX(enable ? 1 : 0));
    if (result != OPUS_OK) return false;
    m_config.enable_dtx = enable;
    return true;
}

bool OpusCodecEncoder::SetFEC(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->encoder) return false;
    int result = opus_encoder_ctl(m_context->encoder, OPUS_SET_INBAND_FEC(enable ? 1 : 0));
    if (result != OPUS_OK) return false;
    m_config.enable_fec = enable;
    return true;
}

uint32_t OpusCodecEncoder::GetFrameSizeSamples() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_context ? m_context->frame_size : 960;
}

OpusCodecDecoder::OpusCodecDecoder()
    : m_context(std::make_unique<DecoderContext>())
    , m_initialized(false) {
}

OpusCodecDecoder::~OpusCodecDecoder() {
    Release();
}

bool OpusCodecDecoder::Initialize(const AudioCodecConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized) { ReleaseInternal(); }

    m_config = config;
    m_context->output_sample_rate = config.sample_rate;
    m_context->channels = config.channels;
    m_context->output_frame_size = (config.sample_rate * config.frame_size_ms) / 1000;
    m_context->need_resample = (config.sample_rate != sc_opus_default_sample_rate);

    int error = 0;
    m_context->decoder = opus_decoder_create(
        sc_opus_default_sample_rate, config.channels, &error);

    if (error != OPUS_OK || !m_context->decoder) {
        LOG_ERROR_STREAM << "[Opus Decoder] FAILED to create decoder: error=" << error;
        return false;
    }

    m_context->sample_buffer.reserve(m_context->output_frame_size * m_context->channels * 4);
    m_context->output_buffer.resize(sc_opus_default_codec_frame_size * m_context->channels);

    if (m_context->need_resample) {
        if (!InitResampler()) { ReleaseInternal(); return false; }
    }

    m_initialized = true;
    return true;
}

bool OpusCodecDecoder::InitResampler() {
    int error = 0;
    m_context->resampler = src_new(SRC_SINC_FASTEST, m_context->channels, &error);
    if (error != 0 || !m_context->resampler) {
        LOG_ERROR_STREAM << "[Opus Decoder] FAILED to create resampler: error=" << error;
        return false;
    }
    return true;
}

void OpusCodecDecoder::ReleaseInternal() {
    if (m_context->resampler) { src_delete(m_context->resampler); m_context->resampler = nullptr; }
    if (m_context->decoder) { opus_decoder_destroy(m_context->decoder); m_context->decoder = nullptr; }
    m_context->sample_buffer.clear();
    while (!m_context->output_queue.empty()) { m_context->output_queue.pop(); }
    m_initialized = false;
}

void OpusCodecDecoder::Release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ReleaseInternal();
}

std::vector<float> OpusCodecDecoder::ResampleAudio(const float* input, size_t input_frames) {
    if (!m_context->need_resample || !m_context->resampler) {
        return std::vector<float>(input, input + input_frames * m_context->channels);
    }
    double ratio = static_cast<double>(m_context->output_sample_rate) / static_cast<double>(sc_opus_default_sample_rate);
    size_t output_frames = static_cast<size_t>(input_frames * ratio + 0.5);
    std::vector<float> output(output_frames * m_context->channels);
    SRC_DATA src_data;
    src_data.data_in = input;
    src_data.input_frames = static_cast<long>(input_frames);
    src_data.data_out = output.data();
    src_data.output_frames = static_cast<long>(output_frames);
    src_data.src_ratio = ratio;
    src_data.end_of_input = 0;
    int error = src_process(m_context->resampler, &src_data);
    if (error != 0) { LOG_ERROR_STREAM << "[Opus Decoder] Resampling error: " << src_strerror(error); return {}; }
    output.resize(src_data.output_frames_gen * m_context->channels);
    return output;
}

void OpusCodecDecoder::CreateOutputFrame(uint64_t timestamp_us) {
    size_t required_samples = m_context->output_frame_size * m_context->channels;
    if (m_context->sample_buffer.size() < required_samples) return;
    RawAudioFrame frame;
    frame.sample_rate = m_context->output_sample_rate;
    frame.channels = m_context->channels;
    frame.sample_per_channel = m_context->output_frame_size;
    frame.timestamp_us = timestamp_us;
    frame.format = AudioSampleFormat::PCM_FLOAT;
    size_t output_size = required_samples * sizeof(float);
    frame.data = std::shared_ptr<uint8_t[]>(new uint8_t[output_size]);
    std::memcpy(frame.data.get(), m_context->sample_buffer.data(), output_size);
    m_context->sample_buffer.erase(
        m_context->sample_buffer.begin(),
        m_context->sample_buffer.begin() + required_samples);
    m_context->output_queue.push(std::move(frame));
}

void OpusCodecDecoder::ProcessBuffer(uint64_t timestamp_us) {
    size_t required_samples = m_context->output_frame_size * m_context->channels;
    while (m_context->sample_buffer.size() >= required_samples) {
        CreateOutputFrame(timestamp_us);
    }
}

bool OpusCodecDecoder::PushInput(const EncodedAudioFrame& input) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->decoder) return false;
    
    int decoded_samples = opus_decode_float(
        m_context->decoder, input.data.data(),
        static_cast<opus_int32>(input.data.size()),
        m_context->output_buffer.data(),
        sc_opus_default_codec_frame_size, 0);
    
    if (decoded_samples < 0) return false;
    
    std::vector<float> process_data;
    const float* data_to_buffer = nullptr;
    size_t frames_to_buffer = 0;
    
    if (m_context->need_resample) {
        process_data = ResampleAudio(m_context->output_buffer.data(), decoded_samples);
        if (process_data.empty()) return false;
        data_to_buffer = process_data.data();
        frames_to_buffer = process_data.size() / m_context->channels;
    } else {
        data_to_buffer = m_context->output_buffer.data();
        frames_to_buffer = decoded_samples;
    }
    
    for (size_t i = 0; i < frames_to_buffer * m_context->channels; ++i) {
        m_context->sample_buffer.push_back(data_to_buffer[i]);
    }
    
    ProcessBuffer(input.timestamp_us);
    m_context->last_timestamp_us = input.timestamp_us;
    return true;
}

bool OpusCodecDecoder::PullDecoded(RawAudioFrame& output) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || m_context->output_queue.empty()) return false;
    output = std::move(m_context->output_queue.front());
    m_context->output_queue.pop();
    return true;
}

size_t OpusCodecDecoder::GetPendingFrameCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_context->output_queue.size();
}

bool OpusCodecDecoder::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    size_t required_samples = m_context->output_frame_size * m_context->channels;
    if (m_context->sample_buffer.size() > 0) {
        RawAudioFrame frame;
        frame.sample_rate = m_context->output_sample_rate;
        frame.channels = m_context->channels;
        frame.sample_per_channel = static_cast<uint32_t>(m_context->sample_buffer.size() / m_context->channels);
        frame.timestamp_us = m_context->last_timestamp_us;
        frame.format = AudioSampleFormat::PCM_FLOAT;
        size_t output_size = m_context->sample_buffer.size() * sizeof(float);
        frame.data = std::shared_ptr<uint8_t[]>(new uint8_t[output_size]);
        std::memcpy(frame.data.get(), m_context->sample_buffer.data(), output_size);
        m_context->output_queue.push(std::move(frame));
        m_context->sample_buffer.clear();
    }
    return true;
}

bool OpusCodecDecoder::ConcealLostPacket(RawAudioFrame& output) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_context->decoder) return false;
    
    int decoded_samples = opus_decode_float(
        m_context->decoder, nullptr, 0,
        m_context->output_buffer.data(),
        sc_opus_default_codec_frame_size, 1);
    
    if (decoded_samples < 0) {
        output.sample_rate = m_context->output_sample_rate;
        output.channels = m_context->channels;
        output.sample_per_channel = m_context->output_frame_size;
        output.format = AudioSampleFormat::PCM_FLOAT;
        size_t output_size = m_context->output_frame_size * m_context->channels * sizeof(float);
        output.data = std::shared_ptr<uint8_t[]>(new uint8_t[output_size]());
        return true;
    }
    
    output.sample_rate = m_context->output_sample_rate;
    output.channels = m_context->channels;
    output.sample_per_channel = decoded_samples;
    output.timestamp_us = 0;
    output.format = AudioSampleFormat::PCM_FLOAT;
    size_t output_size = decoded_samples * m_context->channels * sizeof(float);
    output.data = std::shared_ptr<uint8_t[]>(new uint8_t[output_size]);
    std::memcpy(output.data.get(), m_context->output_buffer.data(), output_size);
    return true;
}

uint32_t OpusCodecDecoder::GetFrameSizeSamples() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_context ? m_context->output_frame_size : 1024;
}

} // namespace CODEC
} // namespace TRANSPORT
