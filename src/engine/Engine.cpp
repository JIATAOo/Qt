#include "Engine.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QRandomGenerator>

Engine* Engine::m_instance = nullptr;

Engine::Engine(QObject *parent)
    : QObject(parent)
{
}

Engine::~Engine()
{
}

Engine* Engine::Instance()
{
    if (!m_instance)
    {
        m_instance = new Engine();
    }
    return m_instance;
}

void Engine::Login(const QString &username, const QString &password)
{
    qDebug() << "Engine::Login - username:" << username;

    if (username.isEmpty() || password.isEmpty())
    {
        emit LoginFailed(tr("Username or password is empty"));
        return;
    }

    // TODO: connect to backend login service
    // For now, simulate successful login
    m_current_username = username;
    qDebug() << "Login success, user:" << username;
    emit LoginSuccess(username);
}

void Engine::Register(const QString &username, const QString &password)
{
    qDebug() << "Engine::Register - username:" << username;

    if (username.isEmpty() || password.isEmpty())
    {
        emit RegisterFailed(tr("Username or password is empty"));
        return;
    }

    // TODO: connect to backend registration service
    // For now, simulate successful registration
    qDebug() << "Register success, user:" << username;
    emit RegisterSuccess();
}

void Engine::CreateMeeting()
{
    qDebug() << "Engine::CreateMeeting - user:" << m_current_username;

    int num = QRandomGenerator::global()->bounded(100000, 999999);
    m_current_meetingId = QString::number(num);

    qDebug() << "Meeting created, ID:" << m_current_meetingId;
    emit MeetingCreated(m_current_meetingId);
}

void Engine::JoinMeeting(const QString &meetingId)
{
    qDebug() << "Engine::JoinMeeting - user:" << m_current_username << ", meeting:" << meetingId;

    if (meetingId.isEmpty())
    {
        qDebug() << "Join meeting failed: empty meeting ID";
        return;
    }

    m_current_meetingId = meetingId;
    qDebug() << "Meeting joined, ID:" << m_current_meetingId;
    emit MeetingJoined(m_current_meetingId);
}

void Engine::LeaveMeeting()
{
    qDebug() << "Engine::LeaveMeeting - meeting:" << m_current_meetingId;
    m_current_meetingId.clear();
    emit MeetingEnded();
}

QString Engine::GetCurrentUsername() const
{
    return m_current_meetingId;
}

QString Engine::GetCurrentMeetingId() const
{
    return m_current_meetingId;
}

bool Engine::LoadSavedCredentials(QString &username, QString &password)
{
    QString file_path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config";
    QDir dir;
    if (!dir.exists(file_path))
    {
        return false;
    }

    QSettings settings(file_path + "/config.ini", QSettings::IniFormat);
    username = settings.value("user_name").toString();
    password = settings.value("user_password").toString();
    return settings.value("rember_password").toBool();
}

void Engine::SaveCredentials(const QString &username, const QString &password)
{
    QString file_path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/config";
    QDir dir;
    if (!dir.exists(file_path))
    {
        dir.mkpath(file_path);
    }

    QSettings settings(file_path + "/config.ini", QSettings::IniFormat);
    settings.setValue("user_name", username);
    settings.setValue("user_password", password);
    settings.setValue("rember_password", true);
}
