#include "H264Codec.h"
#include "Logger.h"
#include <cstring>

extern "C" {
#include "wels/codec_api.h"
#include "wels/codec_def.h"
}

namespace TRANSPORT {
namespace CODEC {

struct H264Encoder::EncoderContext
{
    ISVCEncoder* encoder = nullptr;
    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t framerate = 30;
    uint32_t bitrate_kbps = 2000;
};

struct H264Decoder::DecoderContext
{
    ISVCDecoder* decoder = nullptr;
};

H264Encoder::H264Encoder()
    : m_context(std::make_unique<EncoderContext>())
    , m_initialized(false)
    , m_request_keyframe(false)
{
}

H264Encoder::~H264Encoder()
{
    Release();
}

void H264Encoder::ReleaseInternal()
{
    if (m_context->encoder)
    {
        m_context->encoder->Uninitialize();
        WelsDestroySVCEncoder(m_context->encoder);
        m_context->encoder = nullptr;
    }
    m_initialized = false;
}

void H264Encoder::Release()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ReleaseInternal();
}

bool H264Encoder::Initialize(const VideoCodecConfig& config)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (config.width == 0 || config.height == 0)
    {
        LOG_ERROR("H264Encoder::Initialize: invalid config width/height is 0");
        return false;
    }

    if (m_initialized)
    {
        ReleaseInternal();
    }

    m_config = config;

    if (WelsCreateSVCEncoder(&m_context->encoder) != 0 || !m_context->encoder)
    {
        LOG_ERROR("H264Encoder::Initialize: WelsCreateSVCEncoder failed");
        return false;
    }

    SEncParamExt param;
    m_context->encoder->GetDefaultParams(&param);

    param.iUsageType     = CAMERA_VIDEO_REAL_TIME;
    param.iPicWidth      = static_cast<int>(config.width);
    param.iPicHeight     = static_cast<int>(config.height);
    param.fMaxFrameRate  = static_cast<float>(config.framerate);
    param.iTargetBitrate = static_cast<int>(config.target_bitrate_kbps * 1000);
    param.iMaxBitrate    = static_cast<int>(config.max_bitrate_kbps * 1000);
    param.iRCMode        = RC_BITRATE_MODE;
    param.bEnableFrameSkip = config.enable_frame_dropping ? true : false;

    if (config.threads > 0)
    {
        param.iMultipleThreadIdc = static_cast<int>(config.threads);
    }

    param.iComplexityMode = LOW_COMPLEXITY;
    param.iSpatialLayerNum = 1;
    SSpatialLayerConfig& layer = param.sSpatialLayers[0];
    layer.iVideoWidth        = static_cast<int>(config.width);
    layer.iVideoHeight       = static_cast<int>(config.height);
    layer.fFrameRate         = static_cast<float>(config.framerate);
    layer.iSpatialBitrate    = static_cast<int>(config.target_bitrate_kbps * 1000);
    layer.iMaxSpatialBitrate = static_cast<int>(config.max_bitrate_kbps * 1000);
    layer.uiProfileIdc       = PRO_BASELINE;
    layer.uiLevelIdc         = LEVEL_UNKNOWN;
    layer.iDLayerQp          = 0;

    if (m_context->encoder->InitializeExt(&param) != 0)
    {
        LOG_ERROR("H264Encoder::Initialize: InitializeExt failed");
        WelsDestroySVCEncoder(m_context->encoder);
        m_context->encoder = nullptr;
        return false;
    }

    int video_format = videoFormatI420;
    m_context->encoder->SetOption(ENCODER_OPTION_DATAFORMAT, &video_format);

    int idr_interval = static_cast<int>(config.keyframe_interval);
    m_context->encoder->SetOption(ENCODER_OPTION_IDR_INTERVAL, &idr_interval);

    m_context->width        = config.width;
    m_context->height       = config.height;
    m_context->framerate    = config.framerate;
    m_context->bitrate_kbps = config.target_bitrate_kbps;

    m_initialized = true;
    return true;
}

bool H264Encoder::Encode(const RawVideoFrame& input, EncodedVideoFrame& output)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_context->encoder)
    {
        LOG_ERROR("H264Encoder::Encode: encoder not initialized");
        return false;
    }

    if (!input.IsValid())
    {
        LOG_ERROR("H264Encoder::Encode: invalid input frame");
        return false;
    }

    if (input.format != VideoFormat::I420)
    {
        LOG_ERROR_STREAM << "H264Encoder::Encode: unsupported format "
                         << static_cast<int>(input.format);
        return false;
    }

    if (input.width != m_context->width || input.height != m_context->height)
    {
        LOG_ERROR_STREAM << "H264Encoder::Encode: input frame size "
                         << input.width << "x" << input.height
                         << " does not match encoder config "
                         << m_context->width << "x" << m_context->height;
        return false;
    }

    if (m_request_keyframe)
    {
        int ret = m_context->encoder->ForceIntraFrame(true);
        if (ret == 0)
        {
            m_request_keyframe = false;
        }
        else
        {
            LOG_ERROR_STREAM << "H264Encoder::Encode: ForceIntraFrame failed, ret=" << ret;
        }
    }

    SSourcePicture pic = {};
    pic.iPicWidth     = static_cast<int>(input.width);
    pic.iPicHeight    = static_cast<int>(input.height);
    pic.iColorFormat  = videoFormatI420;
    pic.iStride[0]    = static_cast<int>(input.stride[0] > 0 ? input.stride[0] : input.width);
    pic.iStride[1]    = static_cast<int>(input.stride[1] > 0 ? input.stride[1] : input.width / 2);
    pic.iStride[2]    = static_cast<int>(input.stride[2] > 0 ? input.stride[2] : input.width / 2);
    pic.pData[0] = input.data[0].get();
    pic.pData[1] = input.data[1].get();
    pic.pData[2] = input.data[2].get();

    SFrameBSInfo frame_info = {};
    if (m_context->encoder->EncodeFrame(&pic, &frame_info) != 0)
    {
        LOG_ERROR("H264Encoder::Encode: EncodeFrame failed");
        return false;
    }

    if (frame_info.eFrameType == videoFrameTypeSkip)
    {
        output.data.clear();
        output.is_skip     = true;
        output.timestamp_us   = input.timestamp_us;
        output.width       = input.width;
        output.height      = input.height;
        output.codec_type  = VideoCodecType::H264;
        return true;
    }

    size_t total_size = 0;
    for (int i = 0; i < frame_info.iLayerNum; ++i)
    {
        const SLayerBSInfo& layer = frame_info.sLayerInfo[i];
        for (int j = 0; j < layer.iNalCount; ++j)
        {
            total_size += static_cast<size_t>(layer.pNalLengthInByte[j]);
        }
    }

    if (total_size == 0)
    {
        LOG_ERROR("H264Encoder::Encode: total output size is 0");
        return false;
    }

    output.data.resize(total_size);
    uint8_t* dst = output.data.data();

    for (int i = 0; i < frame_info.iLayerNum; ++i)
    {
        const SLayerBSInfo& layer = frame_info.sLayerInfo[i];
        uint8_t* nal_ptr = layer.pBsBuf;
        for (int j = 0; j < layer.iNalCount; ++j)
        {
            size_t nal_size = static_cast<size_t>(layer.pNalLengthInByte[j]);
            std::memcpy(dst, nal_ptr, nal_size);
            dst += nal_size;
            nal_ptr += nal_size;
        }
    }

    output.is_skip     = false;
    output.timestamp_us   = input.timestamp_us;
    output.width       = input.width;
    output.height      = input.height;
    output.codec_type  = VideoCodecType::H264;

    return true;
}

bool H264Encoder::SetBitrate(uint32_t bitrate_kbps)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_context->encoder)
    {
        LOG_ERROR("H264Encoder::SetBitrate: encoder not initialized");
        return false;
    }

    int bitrate = static_cast<int>(bitrate_kbps * 1000);
    if (m_context->encoder->SetOption(ENCODER_OPTION_BITRATE, &bitrate) != 0)
    {
        LOG_ERROR_STREAM << "H264Encoder::SetBitrate: SetOption failed, bitrate=" << bitrate_kbps;
        return false;
    }

    m_context->bitrate_kbps      = bitrate_kbps;
    m_config.target_bitrate_kbps = bitrate_kbps;
    return true;
}

bool H264Encoder::SetFramerate(uint32_t fps)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_context->encoder)
    {
        LOG_ERROR("H264Encoder::SetFramerate: encoder not initialized");
        return false;
    }

    float frame_rate = static_cast<float>(fps);
    if (m_context->encoder->SetOption(ENCODER_OPTION_FRAME_RATE, &frame_rate) != 0)
    {
        LOG_ERROR_STREAM << "H264Encoder::SetFramerate: SetOption failed, fps=" << fps;
        return false;
    }

    m_context->framerate = fps;
    m_config.framerate   = fps;
    return true;
}

bool H264Encoder::RequestKeyframe()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_request_keyframe = true;
    return true;
}

H264Decoder::H264Decoder()
    : m_context(std::make_unique<DecoderContext>())
    , m_initialized(false)
    , m_request_keyframe(false)
{
}

H264Decoder::~H264Decoder()
{
    Release();
}

void H264Decoder::ReleaseInternal()
{
    if (m_context->decoder)
    {
        m_context->decoder->Uninitialize();
        WelsDestroyDecoder(m_context->decoder);
        m_context->decoder = nullptr;
    }
    m_initialized = false;
}

void H264Decoder::Release()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ReleaseInternal();
}

bool H264Decoder::Initialize(const VideoCodecConfig& config)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (config.width == 0 || config.height == 0)
    {
        LOG_ERROR("H264Decoder::Initialize: invalid config width/height is 0");
        return false;
    }

    if (m_initialized)
    {
        ReleaseInternal();
    }

    m_config = config;

    if (WelsCreateDecoder(&m_context->decoder) != 0 || !m_context->decoder)
    {
        LOG_ERROR("H264Decoder::Initialize: WelsCreateDecoder failed");
        return false;
    }

    SDecodingParam param = {};
    param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
    param.bParseOnly = false;

    if (m_context->decoder->Initialize(&param) != 0)
    {
        LOG_ERROR("H264Decoder::Initialize: decoder Initialize failed");
        WelsDestroyDecoder(m_context->decoder);
        m_context->decoder = nullptr;
        return false;
    }

    m_request_keyframe = false;
    m_initialized      = true;
    return true;
}

bool H264Decoder::Decode(const EncodedVideoFrame& input, RawVideoFrame& output)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_initialized || !m_context->decoder)
    {
        LOG_ERROR("H264Decoder::Decode: decoder not initialized");
        return false;
    }

    uint8_t* plane_data[3] = {nullptr, nullptr, nullptr};
    SBufferInfo buffer_info = {};

    DECODING_STATE result = m_context->decoder->DecodeFrameNoDelay(
        input.data.data(),
        static_cast<int>(input.data.size()),
        plane_data,
        &buffer_info);

    if (result & (dsInvalidArgument | dsOutOfMemory | dsInitialOptExpected))
    {
        LOG_ERROR_STREAM << "H264Decoder::Decode: fatal error, result=" << result;
        return false;
    }

    if (result & (dsRefLost | dsDepLayerLost | dsNoParamSets | dsRefListNullPtrs))
    {
        m_request_keyframe = true;
    }

    if (buffer_info.iBufferStatus != 1)
    {
        if (result != dsErrorFree && result != dsFramePending)
        {
        }
        return false;
    }

    output.width     = static_cast<uint32_t>(buffer_info.UsrData.sSystemBuffer.iWidth);
    output.height    = static_cast<uint32_t>(buffer_info.UsrData.sSystemBuffer.iHeight);
    output.timestamp_us = input.timestamp_us;
    output.format    = VideoFormat::I420;

    uint32_t y_height  = output.height;
    uint32_t uv_height = y_height / 2;

    output.stride[0] = static_cast<uint32_t>(buffer_info.UsrData.sSystemBuffer.iStride[0]);
    output.stride[1] = static_cast<uint32_t>(buffer_info.UsrData.sSystemBuffer.iStride[1]);
    output.stride[2] = static_cast<uint32_t>(buffer_info.UsrData.sSystemBuffer.iStride[1]);

    size_t y_size = output.stride[0] * y_height;
    size_t u_size = output.stride[1] * uv_height;
    size_t v_size = output.stride[2] * uv_height;

    if (plane_data[0] && plane_data[1] && plane_data[2])
    {
        output.data[0] = std::shared_ptr<uint8_t[]>(new uint8_t[y_size]);
        output.data[1] = std::shared_ptr<uint8_t[]>(new uint8_t[u_size]);
        output.data[2] = std::shared_ptr<uint8_t[]>(new uint8_t[v_size]);

        std::memcpy(output.data[0].get(), plane_data[0], y_size);
        std::memcpy(output.data[1].get(), plane_data[1], u_size);
        std::memcpy(output.data[2].get(), plane_data[2], v_size);
    }

    return true;
}

bool H264Decoder::NeedsKeyframe() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_request_keyframe;
}

void H264Decoder::ClearKeyframeRequest()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_request_keyframe = false;
}

} // namespace CODEC
} // namespace TRANSPORT
