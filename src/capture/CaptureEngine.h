#ifndef __CAPTURE_ENGINE_H__
#define __CAPTURE_ENGINE_H__

#include <QObject>
#include <obs.h>
#include <obs.hpp>
#include <memory>
#include <mutex>
#include <atomic>
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

    // 屏幕共享
    bool startScreenShare();
    void stopScreenShare();
    bool isScreenSharing() const;

    // 设备枚举
    using DeviceCb = std::function<bool(const char *id, const char *name)>;
    void enumCameras(DeviceCb cb);
    void enumMics(DeviceCb cb);

    // 切换设备
    void switchCamera(const char *deviceId);
    void switchMic(const char *deviceId);
    std::string currentCameraId() const;
    std::string currentMicId() const;

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
    void createScreenSource();
    void destroySource(const char *name);

    bool initialized_ = false;
    int sampleRate_ = 48000;
    int channels_ = 1;

    // 关机标志：阻止析构后仍有回调 emit 信号
    std::atomic<bool> shuttingDown_{false};

    // 用户选择的设备 ID（空表示自动选第一个）
    std::string camDeviceId_;
    std::string micDeviceId_;

    // OBS 核心
    OBSSceneAutoRelease scene_{};

    // 源引用
    OBSSourceAutoRelease camSource_{};
    OBSSourceAutoRelease micSource_{};
    OBSSourceAutoRelease screenSource_{};

    // 回调锁
    std::mutex callbackMutex_;

    static constexpr const char *kCamSourceName = "vcc_camera";
    static constexpr const char *kMicSourceName = "vcc_mic";
    static constexpr const char *kScreenSourceName = "vcc_screen";
};

} // namespace Capture

#endif // __CAPTURE_ENGINE_H__
