#ifndef __CAPTURE_TYPES_H__
#define __CAPTURE_TYPES_H__

#include <memory>
#include <cstdint>

namespace Capture {

// I420 (YUV420P) planar video frame — three separate planes, compact (no padding)
struct VideoFrame {
    std::shared_ptr<uint8_t[]> data[3];   // y, u, v
    int width = 0;
    int height = 0;
    int64_t timestampUs = 0;              // microseconds

    bool valid() const {
        return data[0] && data[1] && data[2] && width > 0 && height > 0;
    }
    int ySize()    const { return width * height; }
    int uvSize()   const { return (width / 2) * (height / 2); }
    int totalSize() const { return ySize() + uvSize() * 2; }
};

// Float interleaved PCM audio frame
struct AudioFrame {
    std::shared_ptr<uint8_t[]> data;       // raw bytes (float interleaved)
    int samples = 0;
    int channels = 0;
    int sampleRate = 0;
    int64_t timestampUs = 0;

    bool valid() const {
        return data != nullptr && samples > 0 && channels > 0;
    }
    int elementCount() const { return samples * channels; }
    int byteSize()     const { return elementCount() * sizeof(float); }
    // Convenience typed accessor (caller must ensure data != nullptr)
    const float* floats() const { return reinterpret_cast<const float*>(data.get()); }
};

} // namespace Capture

#endif // __CAPTURE_TYPES_H__
