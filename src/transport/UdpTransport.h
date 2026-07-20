#ifndef __UDP_TRANSPORT_H__
#define __UDP_TRANSPORT_H__

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <cstdint>
#include <functional>
#include <vector>

namespace Transport {

// ──── 包类型 ────
enum class PacketType : uint32_t {
    VideoKeyFrame  = 1,   // H.264 关键帧
    VideoDelta     = 2,   // H.264 增量帧
    Audio          = 3,   // Opus 音频
    AudioControl   = 4,   // 音频控制（静音等）
};

// ──── 包头（无分包时） ────
#pragma pack(push, 1)
struct PacketHeader {
    uint32_t type;          // PacketType
    uint32_t sequence;      // 序号
    uint64_t timestampUs;   // 微秒时间戳
    uint32_t payloadSize;   // 负载长度
    // uint8_t payload[payloadSize]; 跟随其后
};
#pragma pack(pop)

static constexpr int kMaxPayloadSize = 65507;        // UDP 最大负载
static constexpr int kHeaderSize     = sizeof(PacketHeader);  // 20 字节
static constexpr int kMaxDataSize    = kMaxPayloadSize - kHeaderSize;

class UdpTransport : public QObject
{
    Q_OBJECT
public:
    explicit UdpTransport(QObject *parent = nullptr);
    ~UdpTransport();

    // 绑定本地端口（用于接收）
    bool bind(quint16 port);

    // 发送编码数据
    bool sendPacket(PacketType type, uint32_t sequence, int64_t timestampUs,
                    const uint8_t *payload, int payloadSize,
                    const QHostAddress &address, quint16 port);

    // 判断是否绑定
    bool isBound() const { return bound_; }

    quint16 localPort() const { return localPort_; }

signals:
    // 收到数据包时发射
    void packetReceived(PacketType type, uint32_t sequence, int64_t timestampUs,
                        const QByteArray &payload, const QHostAddress &sender,
                        quint16 senderPort);
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();

private:
    QUdpSocket *socket_ = nullptr;
    bool bound_ = false;
    quint16 localPort_ = 0;
    uint32_t sendSequence_ = 0;
};

} // namespace Transport

#endif // __UDP_TRANSPORT_H__
