#ifndef __MEETING_ROOM_H__
#define __MEETING_ROOM_H__

#include <QMainWindow>
#include <QImage>
#include <QTimer>

class QFrame;
class ChatPanel;
class ParticipantsPanel;
class QComboBox;

namespace Ui {
class MeetingRoom;
}

namespace Capture {
class CaptureEngine;
class AudioPlayer;
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
    void onParticipantsClicked();
    void onLeaveClicked();
    void onVideoFrame(const Capture::VideoFrame &frame);
    void onAudioFrame(const Capture::AudioFrame &frame);
    void updateVolumeDisplay();
    void captureScreen();
    void onCameraDeviceChanged(int index);
    void onMicDeviceChanged(int index);

private:
    void startCapture();
    void setupVolumeMeter();
    void setupChatPanel();
    void setupParticipantsPanel();
    void setupDeviceSelectors();
    void setupAudioOutput();
    void showSidePanel(QWidget *panel);
    void updateParticipantLabel();
    QImage i420ToImage(const Capture::VideoFrame &frame) const;

    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

    Ui::MeetingRoom *ui;
    QString m_username;
    QString m_meetingId;
    int m_seconds = 0;
    QTimer *m_timer = nullptr;

    // 采集引擎
    Capture::CaptureEngine *capture_ = nullptr;

    // 音频播放器（播放远端音频）
    Capture::AudioPlayer *audioPlayer_ = nullptr;

    // 音量指示器
    QFrame *volumeMeter_ = nullptr;
    QTimer *volumeTimer_ = nullptr;
    float currentVolume_ = 0.0f;

    bool m_isMuted = false;
    bool m_isCameraOn = true;
    bool m_isSharing = false;
    QTimer *m_screenTimer = nullptr;

    // 聊天面板
    ChatPanel *m_chatPanel = nullptr;

    // 参与者面板
    ParticipantsPanel *m_participantsPanel = nullptr;

    // 设备选择器
    QComboBox *m_cameraCombo = nullptr;
    QComboBox *m_micCombo = nullptr;
};

#endif // __MEETING_ROOM_H__
