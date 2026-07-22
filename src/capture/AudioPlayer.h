#ifndef __AUDIO_PLAYER_H__
#define __AUDIO_PLAYER_H__

#include <QObject>
#include <QAudioSink>
#include <QIODevice>
#include <QByteArray>
#include <QMutex>
#include <memory>

namespace Capture {

class AudioPlayer : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer();

    bool init(int sampleRate, int channels);
    void shutdown();
    bool ready() const { return initialized_; }

    // 推送浮点音频数据
    void pushFrame(const float *samples, int count);

    void setVolume(float vol);  // 0.0 ~ 1.0

private:
    QAudioSink *m_sink = nullptr;
    QIODevice *m_device = nullptr;
    QByteArray m_buffer;
    QMutex m_mutex;
    bool initialized_ = false;

    QAudioFormat m_format;
    int m_channels = 1;
    int m_sampleRate = 48000;
};

} // namespace Capture

#endif // __AUDIO_PLAYER_H__
