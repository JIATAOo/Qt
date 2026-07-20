#ifndef __MEETING_ROOM_H__
#define __MEETING_ROOM_H__

#include <QMainWindow>
#include <QImage>
#include <QTimer>

class QFrame;

namespace Ui {
class MeetingRoom;
}

namespace Capture {
class CaptureEngine;
struct VideoFrame;
struct AudioFrame;
}

class MeetingRoom : public QMainWindow
{
    Q_OBJECT
public:
    explicit MeetingRoom(const QString &username, const QString &meetingId, QWidget *parent = nullptr);
    ~MeetingRoom();

signals:
    void meetingEnded();

private slots:
    void onTimerTimeout();
    void onMuteClicked();
    void onCameraClicked();
    void onShareClicked();
    void onChatClicked();
    void onLeaveClicked();
    void onVideoFrame(const Capture::VideoFrame &frame);
    void onAudioFrame(const Capture::AudioFrame &frame);
    void updateVolumeDisplay();

private:
    void startCapture();
    void setupVolumeMeter();
    QImage i420ToImage(const Capture::VideoFrame &frame) const;

    Ui::MeetingRoom *ui;
    QString m_username;
    QString m_meetingId;
    int m_seconds = 0;
    QTimer *m_timer = nullptr;

    // 采集引擎
    Capture::CaptureEngine *capture_ = nullptr;

    // 音量指示器
    QFrame *volumeMeter_ = nullptr;
    QTimer *volumeTimer_ = nullptr;
    float currentVolume_ = 0.0f;

    bool m_isMuted = false;
    bool m_isCameraOn = true;
    bool m_isSharing = false;
};

#endif // __MEETING_ROOM_H__
