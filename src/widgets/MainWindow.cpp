#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Engine.h"
#include "CreateMeetingDialog.h"
#include "JoinMeetingDialog.h"
#include "MeetingRoom.h"
#include <QDebug>

MainWindow::MainWindow(const QString &username, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_username(username)
{
    ui->setupUi(this);

    ui->welcomeLabel->setText(tr("Welcome, %1 !").arg(m_username));

    connect(ui->createBtn, &QPushButton::clicked, [this]() {
        qDebug() << "Create Meeting clicked, user:" << m_username;
        Engine::Instance()->CreateMeeting();
    });

    connect(ui->joinBtn, &QPushButton::clicked, [this]() {
        qDebug() << "Join Meeting clicked, user:" << m_username;
        JoinMeetingDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted)
        {
            Engine::Instance()->JoinMeeting(dlg.GetMeetingId());
        }
    });

    connect(Engine::Instance(), &Engine::MeetingCreated, [this](const QString &meetingId) {
        qDebug() << "Meeting created:" << meetingId;
        CreateMeetingDialog dlg(this);
        dlg.SetMeetingId(meetingId);
        dlg.exec();

        this->hide();
        MeetingRoom *room = new MeetingRoom(m_username, meetingId);
        connect(room, &MeetingRoom::meetingEnded, [this]() {
            this->show();
        });
        room->show();
    });

    connect(Engine::Instance(), &Engine::MeetingJoined, [this](const QString &meetingId) {
        qDebug() << "Meeting joined:" << meetingId;
        this->hide();
        MeetingRoom *room = new MeetingRoom(m_username, meetingId);
        connect(room, &MeetingRoom::meetingEnded, [this]() {
            this->show();
        });
        room->show();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
