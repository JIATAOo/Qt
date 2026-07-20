#include "MeetingRoom.h"
#include "ui_MeetingRoom.h"
#include "Engine.h"
#include "CaptureEngine.h"
#include <QHBoxLayout>
#include <QFrame>
#include <QStyle>
#include <QDebug>
#include <QTime>
#include <cmath>

MeetingRoom::MeetingRoom(const QString &username, const QString &meetingId, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MeetingRoom)
    , m_username(username)
    , m_meetingId(meetingId)
{
    ui->setupUi(this);

    ui->meetingIdLabel->setText(tr("Meeting: %1").arg(m_meetingId));
    ui->videoPlaceholder->setText(tr("Starting camera..."));

    connect(ui->muteBtn, &QPushButton::clicked, this, &MeetingRoom::onMuteClicked);
    connect(ui->cameraBtn, &QPushButton::clicked, this, &MeetingRoom::onCameraClicked);
    connect(ui->shareBtn, &QPushButton::clicked, this, &MeetingRoom::onShareClicked);
    connect(ui->chatBtn, &QPushButton::clicked, this, &MeetingRoom::onChatClicked);
    connect(ui->leaveBtn, &QPushButton::clicked, this, &MeetingRoom::onLeaveClicked);

    // 计时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MeetingRoom::onTimerTimeout);
    m_timer->start(1000);

    // 音量指示器
    setupVolumeMeter();

    // 启动采集
    startCapture();

    qDebug() << "MeetingRoom created for user:" << m_username << ", meeting:" << m_meetingId;
}

MeetingRoom::~MeetingRoom()
{
    if (m_timer)
        m_timer->stop();
    if (volumeTimer_)
        volumeTimer_->stop();
    if (capture_)
        capture_->shutdown();
    delete ui;
}

// ──── 音量指示器 ────

void MeetingRoom::setupVolumeMeter()
{
    auto *parent = ui->centralwidget;
    volumeMeter_ = new QFrame(parent);
    volumeMeter_->setObjectName("volumeMeter");
    volumeMeter_->setFixedSize(56, 6);
    volumeMeter_->raise();

    // 延迟到布局计算完成后定位
    QTimer::singleShot(0, this, [this]() {
        if (!volumeMeter_) return;
        QPoint btnInBar = ui->muteBtn->pos();
        QPoint barPos   = ui->bottomBar->pos();
        volumeMeter_->move(barPos + QPoint(btnInBar.x(), -6));
        volumeMeter_->show();
    });

    volumeTimer_ = new QTimer(this);
    connect(volumeTimer_, &QTimer::timeout, this, &MeetingRoom::updateVolumeDisplay);
    volumeTimer_->start(50);
}

void MeetingRoom::updateVolumeDisplay()
{
    if (!volumeMeter_) return;

    // 做一点平滑衰减：没音量时逐渐归零
    float vol = currentVolume_;
    currentVolume_ *= 0.85f;  // 衰减，让跳动更自然
    if (vol < 0.005f) vol = 0.0f;  // 低于阈值视为静音

    // 根据音量分 7 档（0-6）
    int level = static_cast<int>(vol * 7.0f);
    if (level > 6) level = 6;

    // 颜色：绿(安静) → 黄 → 红(很响)
    static const char *colors[] = {
        "#333333",  // 0: 灰 (静音)
        "#22bb22",  // 1: 绿
        "#66cc22",  // 2: 黄绿
        "#cccc22",  // 3: 黄
        "#dd8811",  // 4: 橙
        "#ee4422",  // 5: 红橙
        "#ff1111",  // 6: 红
    };

    volumeMeter_->setStyleSheet(
        QString("QFrame#volumeMeter { background-color: %1; border: none; border-radius: 2px; }")
            .arg(colors[level]));
}

// ──── 采集 ────

void MeetingRoom::startCapture()
{
    capture_ = new Capture::CaptureEngine(this);

    if (!capture_->init(48000, 1, 1280, 720, 30)) {
        qWarning() << "CaptureEngine init failed";
        ui->videoPlaceholder->setText(tr("Camera unavailable"));
        return;
    }

    connect(capture_, &Capture::CaptureEngine::videoFrameReady,
            this, &MeetingRoom::onVideoFrame);
    connect(capture_, &Capture::CaptureEngine::audioFrameReady,
            this, &MeetingRoom::onAudioFrame);

    capture_->startVideo();
    capture_->startAudio();

    qDebug() << "Capture started";
}

// ──── I420 → RGB ────

QImage MeetingRoom::i420ToImage(const Capture::VideoFrame &frame) const
{
    int w = frame.width;
    int h = frame.height;
    QImage img(w, h, QImage::Format_RGB32);

    const uint8_t *yPlane = frame.data[0].get();
    const uint8_t *uPlane = frame.data[1].get();
    const uint8_t *vPlane = frame.data[2].get();

    for (int row = 0; row < h; ++row) {
        QRgb *dst = reinterpret_cast<QRgb *>(img.scanLine(row));
        for (int col = 0; col < w; ++col) {
            int y = yPlane[row * w + col];
            int u = uPlane[(row / 2) * (w / 2) + col / 2] - 128;
            int v = vPlane[(row / 2) * (w / 2) + col / 2] - 128;

            int r = y + ((1436 * v) >> 10);
            int g = y - ((352 * u + 731 * v) >> 10);
            int b = y + ((1814 * u) >> 10);

            r = r < 0 ? 0 : (r > 255 ? 255 : r);
            g = g < 0 ? 0 : (g > 255 ? 255 : g);
            b = b < 0 ? 0 : (b > 255 ? 255 : b);

            dst[col] = qRgb(r, g, b);
        }
    }
    return img;
}

// ──── 视频帧 ────

void MeetingRoom::onVideoFrame(const Capture::VideoFrame &frame)
{
    if (!frame.valid()) return;

    QImage img = i420ToImage(frame);
    if (!img.isNull()) {
        QPixmap pm = QPixmap::fromImage(img).scaled(
            ui->videoPlaceholder->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->videoPlaceholder->setPixmap(pm);
    }
}

// ──── 音频帧：计算音量 ────

void MeetingRoom::onAudioFrame(const Capture::AudioFrame &frame)
{
    if (!frame.valid() || m_isMuted) return;

    // 计算 RMS
    const float *samples = frame.floats();
    int count = frame.elementCount();
    double sum = 0.0;
    for (int i = 0; i < count; ++i) {
        sum += static_cast<double>(samples[i]) * samples[i];
    }
    float rms = std::sqrt(sum / count);
    float level = rms / 0.3f;
    if (level > 1.0f) level = 1.0f;

    currentVolume_ = level;

    static int dbgCount = 0;
    if (++dbgCount % 50 == 0)
        qDebug() << "[Audio] samples:" << count << "rms:" << rms << "level:" << level;
}

// ──── 计时器 ────

void MeetingRoom::onTimerTimeout()
{
    m_seconds++;
    QTime time(0, 0, 0);
    time = time.addSecs(m_seconds);
    ui->timerLabel->setText(time.toString("mm:ss"));
}

// ──── 静音 ────

void MeetingRoom::onMuteClicked()
{
    m_isMuted = !m_isMuted;
    if (m_isMuted) {
        capture_->stopAudio();
        currentVolume_ = 0.0f;
    } else {
        capture_->startAudio();
    }
    ui->muteBtn->setText(m_isMuted ? tr("Unmute") : tr("Mute"));
    ui->muteBtn->setProperty("muted", m_isMuted);
    ui->muteBtn->style()->unpolish(ui->muteBtn);
    ui->muteBtn->style()->polish(ui->muteBtn);
    qDebug() << "Mute toggled:" << m_isMuted;
}

// ──── 摄像头 ────

void MeetingRoom::onCameraClicked()
{
    m_isCameraOn = !m_isCameraOn;
    if (m_isCameraOn) {
        capture_->startVideo();
    } else {
        capture_->stopVideo();
        ui->videoPlaceholder->setPixmap(QPixmap());
        ui->videoPlaceholder->setText(tr("Camera off"));
    }
    ui->cameraBtn->setProperty("cameraOn", m_isCameraOn);
    ui->cameraBtn->style()->unpolish(ui->cameraBtn);
    ui->cameraBtn->style()->polish(ui->cameraBtn);
    qDebug() << "Camera toggled:" << m_isCameraOn;
}

// ──── 共享 ────

void MeetingRoom::onShareClicked()
{
    m_isSharing = !m_isSharing;
    ui->shareBtn->setProperty("sharing", m_isSharing);
    ui->shareBtn->style()->unpolish(ui->shareBtn);
    ui->shareBtn->style()->polish(ui->shareBtn);
    qDebug() << "Share toggled:" << m_isSharing;
}

// ──── 聊天 ────

void MeetingRoom::onChatClicked()
{
    qDebug() << "Chat button clicked - TODO: show chat panel";
}

// ──── 离开 ────

void MeetingRoom::onLeaveClicked()
{
    qDebug() << "User" << m_username << "leaving meeting" << m_meetingId;
    Engine::Instance()->LeaveMeeting();
    emit meetingEnded();
    close();
}
