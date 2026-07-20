#include "NetworkEngine.h"
#include "Logger.h"

namespace Network {

NetworkEngine::NetworkEngine(QObject *parent)
    : QObject(parent)
{
}

NetworkEngine::~NetworkEngine()
{
    shutdown();
}

bool NetworkEngine::init(int videoWidth, int videoHeight, int videoFps,
                          int audioSampleRate, int audioChannels,
                          int localUdpPort)
{
    if (initialized_) return true;

    videoWidth_  = videoWidth;
    videoHeight_ = videoHeight;

    // 1. 创建视频编码器
    videoEncoder_ = std::make_unique<Codec::VideoEncoder>();
    if (!videoEncoder_->init(videoWidth, videoHeight, videoFps)) {
        LOG_ERROR("Video encoder init failed");
        emit errorOccurred(tr("Video encoder init failed"));
        return false;
    }
    LOG_INFO_S << "Video encoder initialized: " << videoWidth << "x" << videoHeight
               << "@" << videoFps << "fps";

    // 2. 创建音频编码器
    audioEncoder_ = std::make_unique<Codec::AudioEncoder>();
    if (!audioEncoder_->init(audioSampleRate, audioChannels)) {
        LOG_ERROR("Audio encoder init failed");
        emit errorOccurred(tr("Audio encoder init failed"));
        return false;
    }
    LOG_INFO_S << "Audio encoder initialized: " << audioSampleRate << "Hz, "
               << audioChannels << "ch";

    // 3. 创建 UDP 传输层
    transport_ = std::make_unique<Transport::UdpTransport>();
    if (!transport_->bind(localUdpPort)) {
        LOG_ERROR("UDP transport bind failed");
        emit errorOccurred(tr("UDP transport bind failed"));
        return false;
    }

    connect(transport_.get(), &Transport::UdpTransport::packetReceived,
            this, &NetworkEngine::onPacketReceived);
    connect(transport_.get(), &Transport::UdpTransport::errorOccurred,
            this, &NetworkEngine::errorOccurred);

    LOG_INFO_S << "UDP transport bound on port " << transport_->localPort();

    initialized_ = true;
    return true;
}

void NetworkEngine::shutdown()
{
    if (!initialized_) return;

    videoEncoder_.reset();
    audioEncoder_.reset();
    transport_.reset();

    initialized_ = false;
    LOG_INFO("NetworkEngine shutdown");
}

void NetworkEngine::setRemoteAddress(const QString &address, quint16 port)
{
    remoteAddr_.setAddress(address);
    remotePort_ = port;
    LOG_INFO_S << "Remote address set: " << address.toStdString() << ":" << port;
}

void NetworkEngine::pushVideoFrame(const Capture::VideoFrame &frame)
{
    if (!initialized_ || !videoEncoder_ || !transport_) return;
    if (remotePort_ == 0) return;  // 还没有设置远端地址

    // H.264 编码
    auto encoded = videoEncoder_->encode(
        frame.data[0].get(), frame.data[1].get(), frame.data[2].get(),
        frame.width, frame.height, frame.timestampUs);

    if (!encoded.valid()) return;

    // UDP 发送
    uint32_t seq = videoSeq_.fetch_add(1);
    auto type = encoded.isKeyFrame ? Transport::PacketType::VideoKeyFrame
                                   : Transport::PacketType::VideoDelta;

    transport_->sendPacket(type, seq, encoded.timestampUs,
                           encoded.data.get(), encoded.size,
                           remoteAddr_, remotePort_);
}

void NetworkEngine::pushAudioFrame(const Capture::AudioFrame &frame)
{
    if (!initialized_ || !audioEncoder_ || !transport_) return;
    if (remotePort_ == 0) return;  // 还没有设置远端地址

    // Opus 编码
    auto encoded = audioEncoder_->encode(
        frame.floats(), frame.samples, frame.channels, frame.timestampUs);

    if (!encoded.valid()) return;

    // UDP 发送
    uint32_t seq = audioSeq_.fetch_add(1);

    transport_->sendPacket(Transport::PacketType::Audio, seq, encoded.timestampUs,
                           encoded.data.get(), encoded.size,
                           remoteAddr_, remotePort_);
}

void NetworkEngine::onPacketReceived(Transport::PacketType type, uint32_t sequence,
                                      int64_t timestampUs, const QByteArray &payload,
                                      const QHostAddress &sender, quint16 senderPort)
{
    Q_UNUSED(sequence);
    Q_UNUSED(sender);
    Q_UNUSED(senderPort);

    switch (type) {
    case Transport::PacketType::VideoKeyFrame:
    case Transport::PacketType::VideoDelta: {
        bool isKey = (type == Transport::PacketType::VideoKeyFrame);
        emit remoteVideoFrameReady(payload, isKey, timestampUs);
        break;
    }
    case Transport::PacketType::Audio: {
        emit remoteAudioFrameReady(payload, timestampUs);
        break;
    }
    default:
        break;
    }
}

} // namespace Network
