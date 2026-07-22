#include "MeetingRoom.h"
#include "ui_MeetingRoom.h"
#include "Engine.h"
#include "CaptureEngine.h"
#include "ChatPanel.h"
#include "ParticipantsPanel.h"
#include "AudioPlayer.h"
#include <QHBoxLayout>
#include <QFrame>
#include <QStyle>
#include <QDebug>
#include <QTime>
#include <QResizeEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QComboBox>
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

    // 参与者标签点击
    ui->participantsLabel->setCursor(Qt::PointingHandCursor);
    ui->participantsLabel->installEventFilter(this);

    // 计时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MeetingRoom::onTimerTimeout);
    m_timer->start(1000);

    // 音量指示器
    setupVolumeMeter();

    // 聊天面板
    setupChatPanel();

    // 参与者面板
    setupParticipantsPanel();

    // 启动采集
    startCapture();

    // 音频输出（播放远端音频）
    setupAudioOutput();

    // 设备选择器（必须在 startCapture 之后，因为需要 capture_ 来枚举设备）
    setupDeviceSelectors();

    qDebug() << "MeetingRoom created for user:" << m_username << ", meeting:" << m_meetingId;
}

MeetingRoom::~MeetingRoom()
{
    // 先停止定时器
    if (m_timer) {
        m_timer->stop();
        m_timer = nullptr;
    }
    if (volumeTimer_) {
        volumeTimer_->stop();
        volumeTimer_ = nullptr;
    }
    if (m_screenTimer) {
        m_screenTimer->stop();
        m_screenTimer = nullptr;
    }

    // 再停止采集（阻止新的 OBS 回调）
    if (capture_) {
        capture_->disconnect(this);
        capture_->shutdown();
        capture_ = nullptr;
    }

    if (audioPlayer_) {
        audioPlayer_->shutdown();
        audioPlayer_ = nullptr;
    }

    volumeMeter_ = nullptr;
    m_chatPanel = nullptr;
    m_participantsPanel = nullptr;
    m_cameraCombo = nullptr;
    m_micCombo = nullptr;
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
    // 屏幕共享时不渲染 OBS 视频帧
    if (m_isSharing) return;
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

    if (m_isSharing) {
        // 使用 Qt 屏幕捕获替代 OBS 的 win-capture 插件
        if (!m_screenTimer) {
            m_screenTimer = new QTimer(this);
            connect(m_screenTimer, &QTimer::timeout, this, &MeetingRoom::captureScreen);
        }
        m_screenTimer->start(33);  // ~30fps
        qDebug() << "Screen sharing started (Qt)";
    } else {
        if (m_screenTimer)
            m_screenTimer->stop();
        // 恢复摄像头画面（如果有的话）
        if (!m_isCameraOn) {
            ui->videoPlaceholder->setText(tr("Camera off"));
        }
        qDebug() << "Screen sharing stopped (Qt)";
    }

    ui->shareBtn->setProperty("sharing", m_isSharing);
    ui->shareBtn->style()->unpolish(ui->shareBtn);
    ui->shareBtn->style()->polish(ui->shareBtn);
}

void MeetingRoom::captureScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QPixmap pm = screen->grabWindow(0);
    if (!pm.isNull()) {
        ui->videoPlaceholder->setPixmap(pm.scaled(
            ui->videoPlaceholder->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

// ──── 聊天 ────

void MeetingRoom::onChatClicked()
{
    if (!m_chatPanel) return;

    if (m_chatPanel->isVisible()) {
        m_chatPanel->hide();
    } else {
        showSidePanel(m_chatPanel);
    }
    qDebug() << "Chat panel toggled:" << m_chatPanel->isVisible();
}

// ──── 参与者 ────

void MeetingRoom::onParticipantsClicked()
{
    if (!m_participantsPanel) return;

    if (m_participantsPanel->isVisible()) {
        m_participantsPanel->hide();
    } else {
        showSidePanel(m_participantsPanel);
    }
    qDebug() << "Participants panel toggled:" << m_participantsPanel->isVisible();
}

// ──── 侧边栏切换 ────

void MeetingRoom::showSidePanel(QWidget *panel)
{
    if (!panel || !ui->centralwidget) return;

    // 隐藏所有侧边面板
    if (m_chatPanel && m_chatPanel != panel)
        m_chatPanel->hide();
    if (m_participantsPanel && m_participantsPanel != panel)
        m_participantsPanel->hide();

    // 定位并显示目标面板
    int w = panel->width();
    int h = ui->centralwidget->height();
    int x = ui->centralwidget->width() - w;
    panel->setGeometry(x, 0, w, h);
    panel->show();
    panel->raise();
}

// ──── 离开 ────

void MeetingRoom::onLeaveClicked()
{
    qDebug() << "User" << m_username << "leaving meeting" << m_meetingId;
    Engine::Instance()->LeaveMeeting();
    emit meetingEnded();
    close();
}

// ──── 聊天面板 ────

void MeetingRoom::setupChatPanel()
{
    m_chatPanel = new ChatPanel(m_username, ui->centralwidget);
    m_chatPanel->hide();

    connect(m_chatPanel, &ChatPanel::messageSent, this, [this](const QString &sender, const QString &msg) {
        qDebug() << "[Chat]" << sender << ":" << msg;
        // TODO: 后续通过网络发送消息给其他参与者
    });
}

// ──── 参与者面板 ────

void MeetingRoom::setupParticipantsPanel()
{
    m_participantsPanel = new ParticipantsPanel(ui->centralwidget);
    m_participantsPanel->hide();

    // 初始参与者列表
    QStringList participants;
    participants << m_username;
    m_participantsPanel->setParticipants(participants);
    updateParticipantLabel();
}

// ──── 设备选择器 ────

void MeetingRoom::setupDeviceSelectors()
{
    if (!capture_) return;

    QHBoxLayout *topLayout = qobject_cast<QHBoxLayout *>(ui->topBar->layout());
    if (!topLayout) return;

    // 摄像头选择
    m_cameraCombo = new QComboBox(ui->topBar);
    m_cameraCombo->setObjectName("cameraCombo");
    m_cameraCombo->setMinimumWidth(130);
    m_cameraCombo->setMaximumWidth(180);
    m_cameraCombo->setToolTip(tr("Camera"));

    capture_->enumCameras([this](const char *id, const char *name) {
        m_cameraCombo->addItem(QString::fromUtf8(name), QString::fromUtf8(id));
        return true;
    });

    connect(m_cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MeetingRoom::onCameraDeviceChanged);

    // 麦克风选择
    m_micCombo = new QComboBox(ui->topBar);
    m_micCombo->setObjectName("micCombo");
    m_micCombo->setMinimumWidth(130);
    m_micCombo->setMaximumWidth(180);
    m_micCombo->setToolTip(tr("Microphone"));

    capture_->enumMics([this](const char *id, const char *name) {
        m_micCombo->addItem(QString::fromUtf8(name), QString::fromUtf8(id));
        return true;
    });

    connect(m_micCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MeetingRoom::onMicDeviceChanged);

    topLayout->addWidget(m_cameraCombo);
    topLayout->addWidget(m_micCombo);
}

void MeetingRoom::onCameraDeviceChanged(int index)
{
    if (!capture_ || !m_cameraCombo || index < 0) return;

    QByteArray id = m_cameraCombo->itemData(index).toString().toUtf8();
    capture_->switchCamera(id.constData());
    qDebug() << "Camera changed to:" << m_cameraCombo->currentText();
}

void MeetingRoom::onMicDeviceChanged(int index)
{
    if (!capture_ || !m_micCombo || index < 0) return;

    QByteArray id = m_micCombo->itemData(index).toString().toUtf8();
    capture_->switchMic(id.constData());
    qDebug() << "Mic changed to:" << m_micCombo->currentText();
}

// ──── 音频输出 ────

void MeetingRoom::setupAudioOutput()
{
    audioPlayer_ = new Capture::AudioPlayer(this);
    if (!audioPlayer_->init(48000, 1)) {
        qWarning() << "AudioPlayer init failed";
        audioPlayer_ = nullptr;
        return;
    }

    // 连接音频帧信号：远端音频到达时自动播放
    // 当前无远端流，播放器就绪待用
    qDebug() << "Audio output ready";
}

void MeetingRoom::updateParticipantLabel()
{
    if (!m_participantsPanel) return;
    int count = m_participantsPanel->participantCount();
    ui->participantsLabel->setText(count == 1
        ? tr("1 participant")
        : tr("%1 participants").arg(count));
}

// ──── 事件过滤（处理 participantsLabel 点击） ────

bool MeetingRoom::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->participantsLabel && event->type() == QEvent::MouseButtonPress) {
        onParticipantsClicked();
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

// ──── resizeEvent ────

void MeetingRoom::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    // 重新定位可见的侧边面板
    QWidget *visiblePanel = nullptr;
    if (m_chatPanel && m_chatPanel->isVisible())
        visiblePanel = m_chatPanel;
    else if (m_participantsPanel && m_participantsPanel->isVisible())
        visiblePanel = m_participantsPanel;

    if (visiblePanel) {
        int w = visiblePanel->width();
        int h = ui->centralwidget->height();
        int x = ui->centralwidget->width() - w;
        visiblePanel->setGeometry(x, 0, w, h);
    }
}
