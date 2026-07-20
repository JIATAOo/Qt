#include "UdpTransport.h"
#include <QNetworkDatagram>

namespace Transport {

UdpTransport::UdpTransport(QObject *parent)
    : QObject(parent)
{
    socket_ = new QUdpSocket(this);
    connect(socket_, &QUdpSocket::readyRead, this, &UdpTransport::onReadyRead);
}

UdpTransport::~UdpTransport()
{
    if (socket_) {
        socket_->close();
    }
}

bool UdpTransport::bind(quint16 port)
{
    if (bound_) return true;

    if (!socket_->bind(QHostAddress::AnyIPv4, port)) {
        emit errorOccurred(tr("UDP bind failed on port %1").arg(port));
        return false;
    }

    localPort_ = socket_->localPort();
    bound_     = true;
    return true;
}

bool UdpTransport::sendPacket(PacketType type, uint32_t sequence,
                               int64_t timestampUs, const uint8_t *payload,
                               int payloadSize,
                               const QHostAddress &address, quint16 port)
{
    if (!socket_) return false;

    // 构造包头
    PacketHeader header;
    header.type        = static_cast<uint32_t>(type);
    header.sequence    = sequence;
    header.timestampUs = static_cast<uint64_t>(timestampUs);
    header.payloadSize = static_cast<uint32_t>(payloadSize);

    QByteArray datagram;
    datagram.resize(sizeof(header) + payloadSize);

    // 拷贝包头
    std::memcpy(datagram.data(), &header, sizeof(header));

    // 拷贝负载
    if (payloadSize > 0 && payload) {
        std::memcpy(datagram.data() + sizeof(header), payload, payloadSize);
    }

    qint64 sent = socket_->writeDatagram(datagram, address, port);
    return sent == datagram.size();
}

void UdpTransport::onReadyRead()
{
    while (socket_->hasPendingDatagrams()) {
        QNetworkDatagram datagram = socket_->receiveDatagram();
        QByteArray data = datagram.data();

        if (data.size() < (int)kHeaderSize) continue;

        const PacketHeader *header = reinterpret_cast<const PacketHeader *>(data.constData());

        uint32_t payloadSize = header->payloadSize;
        if (data.size() < (int)(kHeaderSize + payloadSize)) continue;

        QByteArray payload(data.constData() + kHeaderSize, payloadSize);

        emit packetReceived(
            static_cast<PacketType>(header->type),
            header->sequence,
            static_cast<int64_t>(header->timestampUs),
            payload,
            datagram.senderAddress(),
            datagram.senderPort());
    }
}

} // namespace Transport
