#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include <cstdint>
#include <vector>
#include <memory>

struct ISVCEncoder;

namespace Codec {

struct EncodedVideoFrame {
    std::shared_ptr<uint8_t[]> data;
    int size = 0;
    int64_t timestampUs = 0;
    bool isKeyFrame = false;

    bool valid() const { return data != nullptr && size > 0; }
};

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    // 初始化编码器
    bool init(int width, int height, int fps, int bitrateKbps = 2000);

    // 编码一帧 I420，返回 Annex B 格式的 H.264 数据
    EncodedVideoFrame encode(const uint8_t *yPlane, const uint8_t *uPlane,
                             const uint8_t *vPlane, int width, int height,
                             int64_t timestampUs);

    // 强制下一帧为关键帧
    void requestKeyFrame();

    // 动态调整码率
    void setBitrate(int bitrateKbps);

    bool ready() const { return initialized_; }

private:
    ISVCEncoder *encoder_ = nullptr;
    bool initialized_ = false;
    int width_ = 0;
    int height_ = 0;
    int fps_ = 0;
    bool forceKeyFrame_ = true;  // 第一帧必须是关键帧
};

} // namespace Codec

#endif // __VIDEO_ENCODER_H__
