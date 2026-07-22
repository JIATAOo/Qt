#include "AudioPlayer.h"
#include <QAudioDevice>
#include <QMediaDevices>
#include <QDebug>

namespace Capture {

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject(parent)
{
}

AudioPlayer::~AudioPlayer()
{
    shutdown();
}

bool AudioPlayer::init(int sampleRate, int channels)
{
    if (initialized_) return true;

    m_sampleRate = sampleRate;
    m_channels = channels;

    m_format.setSampleRate(sampleRate);
    m_format.setChannelCount(channels);
    m_format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    if (!defaultDevice.isFormatSupported(m_format)) {
        qWarning() << "AudioPlayer: float format not supported, trying int16";
        m_format.setSampleFormat(QAudioFormat::Int16);
        if (!defaultDevice.isFormatSupported(m_format)) {
            qWarning() << "AudioPlayer: no supported format";
            return false;
        }
    }

    m_sink = new QAudioSink(defaultDevice, m_format, this);
    m_sink->setBufferSize(sampleRate * channels * sizeof(float) / 10);  // ~100ms buffer

    m_device = m_sink->start();
    if (!m_device) {
        qWarning() << "AudioPlayer: failed to start audio sink";
        return false;
    }

    initialized_ = true;
    qDebug() << "AudioPlayer initialized:" << sampleRate << "Hz," << channels << "ch";
    return true;
}

void AudioPlayer::shutdown()
{
    if (!initialized_) return;

    if (m_sink) {
        m_sink->stop();
        m_sink = nullptr;
        m_device = nullptr;
    }

    initialized_ = false;
    qDebug() << "AudioPlayer shutdown";
}

void AudioPlayer::pushFrame(const float *samples, int count)
{
    if (!initialized_ || !m_device || !samples || count <= 0) return;

    QMutexLocker lock(&m_mutex);

    if (m_format.sampleFormat() == QAudioFormat::Float) {
        const char *data = reinterpret_cast<const char *>(samples);
        m_device->write(data, count * sizeof(float));
    } else {
        // 转换为 Int16
        QByteArray converted;
        converted.resize(count * sizeof(int16_t));
        auto *dst = reinterpret_cast<int16_t *>(converted.data());
        for (int i = 0; i < count; ++i) {
            float v = samples[i] * 32767.0f;
            if (v > 32767.0f) v = 32767.0f;
            if (v < -32768.0f) v = -32768.0f;
            dst[i] = static_cast<int16_t>(v);
        }
        m_device->write(converted);
    }
}

void AudioPlayer::setVolume(float vol)
{
    if (m_sink) {
        vol = vol < 0.0f ? 0.0f : (vol > 1.0f ? 1.0f : vol);
        m_sink->setVolume(vol);
    }
}

} // namespace Capture
