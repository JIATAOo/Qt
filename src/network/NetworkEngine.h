#ifndef __NETWORK_ENGINE_H__
#define __NETWORK_ENGINE_H__

#include <QObject>
#include <QHostAddress>
#include <memory>
#include <atomic>

#include "CaptureTypes.h"
#include "VideoEncoder.h"
#include "AudioEncoder.h"
#include "UdpTransport.h"

namespace Network {

class NetworkEngine : public QObject
{
    Q_OBJECT
public:
    explicit NetworkEngine(QObject *parent = nullptr);
    ~NetworkEngine();

    // 初始化编码器和传输层
    bool init(int videoWidth, int videoHeight, int videoFps,
              int audioSampleRate, int audioChannels,
              int localUdpPort);

    void shutdown();

    // 设置远端地址
    void setRemoteAddress(const QString &address, quint16 port);

    // 投递采集帧（从 CaptureEngine 的 signal 连接过来）
    void pushVideoFrame(const Capture::VideoFrame &frame);
    void pushAudioFrame(const Capture::AudioFrame &frame);

    bool ready() const { return initialized_; }

signals:
    // 远端数据到达
    void remoteVideoFrameReady(const QByteArray &h264Data, bool isKeyFrame,
                               int64_t timestampUs);
    void remoteAudioFrameReady(const QByteArray &opusData, int64_t timestampUs);
    void errorOccurred(const QString &message);

private slots:
    void onPacketReceived(Transport::PacketType type, uint32_t sequence,
                          int64_t timestampUs, const QByteArray &payload,
                          const QHostAddress &sender, quint16 senderPort);

private:
    bool initialized_ = false;

    // 编解码器
    std::unique_ptr<Codec::VideoEncoder> videoEncoder_;
    std::unique_ptr<Codec::AudioEncoder> audioEncoder_;

    // 传输
    std::unique_ptr<Transport::UdpTransport> transport_;

    // 远端地址
    QHostAddress remoteAddr_;
    quint16 remotePort_ = 0;

    // 序列号
    std::atomic<uint32_t> videoSeq_{0};
    std::atomic<uint32_t> audioSeq_{0};

    // 本地参数
    int videoWidth_  = 640;
    int videoHeight_ = 480;
};

} // namespace Network

#endif // __NETWORK_ENGINE_H__
