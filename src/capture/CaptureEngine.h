#ifndef __CAPTURE_ENGINE_H__
#define __CAPTURE_ENGINE_H__

#include <QObject>
#include <obs.h>
#include <obs.hpp>
#include <memory>
#include <mutex>
#include <functional>
#include "CaptureTypes.h"

namespace Capture {

class CaptureEngine : public QObject
{
    Q_OBJECT
public:
    explicit CaptureEngine(QObject *parent = nullptr);
    ~CaptureEngine();

    // 生命周期
    bool init(int sampleRate, int channels, int width, int height, int fps);
    void shutdown();
    bool ready() const;

    // 摄像头
    bool startVideo();
    void stopVideo();
    bool videoRunning() const;

    // 麦克风
    bool startAudio();
    void stopAudio();
    bool audioRunning() const;

    // 停止全部
    void stopAll();

    // 设备枚举
    using DeviceCb = std::function<bool(const char *id, const char *name)>;
    void enumCameras(DeviceCb cb);
    void enumMics(DeviceCb cb);

signals:
    void videoFrameReady(const VideoFrame &frame);
    void audioFrameReady(const AudioFrame &frame);
    void errorOccurred(const QString &message);

private:
    // OBS 静态回调
    static void onRawVideo(void *param, video_data *frame);
    static void onRawAudio(void *param, size_t mixIdx, audio_data *data);

    // 帧转换
    VideoFrame convertVideo(const video_data *obsFrame);
    AudioFrame convertAudio(const audio_data *obsData);

    // 源创建
    void createCameraSource();
    void createMicSource();
    void destroySource(const char *name);

    bool initialized_ = false;
    int sampleRate_ = 48000;
    int channels_ = 1;

    // OBS 核心
    OBSSceneAutoRelease scene_{};

    // 源引用
    OBSSourceAutoRelease camSource_{};
    OBSSourceAutoRelease micSource_{};

    // 回调锁
    std::mutex callbackMutex_;

    static constexpr const char *kCamSourceName = "vcc_camera";
    static constexpr const char *kMicSourceName = "vcc_mic";
};

} // namespace Capture

#endif // __CAPTURE_ENGINE_H__
