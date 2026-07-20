#include "VideoEncoder.h"

extern "C" {
#include "wels/codec_api.h"
#include "wels/codec_def.h"
}

#include <cstring>

namespace Codec {

VideoEncoder::VideoEncoder() = default;

VideoEncoder::~VideoEncoder()
{
    if (encoder_) {
        encoder_->Uninitialize();
        WelsDestroySVCEncoder(encoder_);
        encoder_ = nullptr;
    }
}

bool VideoEncoder::init(int width, int height, int fps, int bitrateKbps)
{
    if (initialized_) return true;

    width_  = width;
    height_ = height;
    fps_    = fps;

    if (WelsCreateSVCEncoder(&encoder_) != 0) {
        return false;
    }

    SEncParamExt param;
    encoder_->GetDefaultParams(&param);

    param.iUsageType       = CAMERA_VIDEO_REAL_TIME;
    param.iPicWidth        = width;
    param.iPicHeight       = height;
    param.iTargetBitrate   = bitrateKbps * 1000;
    param.iMaxBitrate      = bitrateKbps * 1500;
    param.iRCMode          = RC_BITRATE_MODE;
    param.fMaxFrameRate    = (float)fps;
    param.iTemporalLayerNum = 1;
    param.iSpatialLayerNum  = 1;
    param.bEnableDenoise   = false;
    param.bEnableBackgroundDetection = false;
    param.bEnableAdaptiveQuant = false;
    param.bEnableFrameSkip = true;
    param.bEnableLongTermReference = false;
    param.iLtrMarkPeriod   = 30;
    param.uiIntraPeriod    = fps * 2;  // 每2秒一个关键帧
    param.iComplexityMode  = LOW_COMPLEXITY;

    // 空间层配置
    param.sSpatialLayers[0].iVideoWidth        = width;
    param.sSpatialLayers[0].iVideoHeight       = height;
    param.sSpatialLayers[0].fFrameRate         = (float)fps;
    param.sSpatialLayers[0].iSpatialBitrate    = bitrateKbps * 1000;
    param.sSpatialLayers[0].iMaxSpatialBitrate = bitrateKbps * 1500;
    param.sSpatialLayers[0].uiProfileIdc       = PRO_BASELINE;
    param.sSpatialLayers[0].uiLevelIdc         = LEVEL_3_1;

    if (encoder_->InitializeExt(&param) != 0) {
        WelsDestroySVCEncoder(encoder_);
        encoder_ = nullptr;
        return false;
    }

    initialized_ = true;
    forceKeyFrame_ = true;  // 第一帧强制关键帧
    return true;
}

EncodedVideoFrame VideoEncoder::encode(const uint8_t *yPlane, const uint8_t *uPlane,
                                        const uint8_t *vPlane, int width, int height,
                                        int64_t timestampUs)
{
    EncodedVideoFrame result;
    if (!initialized_ || !encoder_) return result;

    // 构造源图像
    SSourcePicture pic;
    std::memset(&pic, 0, sizeof(pic));
    pic.iPicWidth    = width;
    pic.iPicHeight   = height;
    pic.iColorFormat = videoFormatI420;
    pic.iStride[0]   = width;
    pic.iStride[1]   = width / 2;
    pic.iStride[2]   = width / 2;
    pic.pData[0]     = const_cast<uint8_t *>(yPlane);
    pic.pData[1]     = const_cast<uint8_t *>(uPlane);
    pic.pData[2]     = const_cast<uint8_t *>(vPlane);

    // 强制关键帧
    if (forceKeyFrame_) {
        encoder_->ForceIntraFrame(true);
        forceKeyFrame_ = false;
    }

    SFrameBSInfo info;
    std::memset(&info, 0, sizeof(info));

    int ret = encoder_->EncodeFrame(&pic, &info);
    if (ret != cmResultSuccess && ret != dsOutOfMemory) {
        return result;
    }

    if (info.eFrameType == videoFrameTypeSkip) {
        return result;  // 编码器跳过了这帧
    }

    result.timestampUs = timestampUs;
    result.isKeyFrame  = (info.eFrameType == videoFrameTypeIDR);

    // 计算总大小
    int totalSize = 0;
    for (int layer = 0; layer < info.iLayerNum; ++layer) {
        const SLayerBSInfo &layerInfo = info.sLayerInfo[layer];
        for (int i = 0; i < layerInfo.iNalCount; ++i) {
            totalSize += layerInfo.pNalLengthInByte[i];
        }
    }

    if (totalSize <= 0) return result;

    result.data = std::shared_ptr<uint8_t[]>(new uint8_t[totalSize]);
    result.size = totalSize;

    // 拷贝 NAL 单元数据
    int offset = 0;
    for (int layer = 0; layer < info.iLayerNum; ++layer) {
        const SLayerBSInfo &layerInfo = info.sLayerInfo[layer];
        int layerDataSize = 0;
        for (int i = 0; i < layerInfo.iNalCount; ++i) {
            layerDataSize += layerInfo.pNalLengthInByte[i];
        }
        std::memcpy(result.data.get() + offset, layerInfo.pBsBuf, layerDataSize);
        offset += layerDataSize;
    }

    return result;
}

void VideoEncoder::requestKeyFrame()
{
    forceKeyFrame_ = true;
}

void VideoEncoder::setBitrate(int bitrateKbps)
{
    if (encoder_) {
        SBitrateInfo brInfo;
        brInfo.iLayer   = SPATIAL_LAYER_ALL;
        brInfo.iBitrate = bitrateKbps * 1000;
        encoder_->SetOption(ENCODER_OPTION_BITRATE, &brInfo);
    }
}

} // namespace Codec
