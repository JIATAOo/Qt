#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <QObject>
#include <QString>

class Engine : public QObject
{
    Q_OBJECT
public:
    static Engine* Instance();

    void Login(const QString &username, const QString &password);
    void Register(const QString &username, const QString &password);
    void CreateMeeting();
    void JoinMeeting(const QString &meetingId);
    void LeaveMeeting();

    QString GetCurrentUsername() const;
    QString GetCurrentMeetingId() const;

    bool LoadSavedCredentials(QString &username, QString &password);
    void SaveCredentials(const QString &username, const QString &password);

signals:
    void LoginSuccess(const QString &username);
    void LoginFailed(const QString &reason);
    void RegisterSuccess();
    void RegisterFailed(const QString &reason);
    void MeetingCreated(const QString &meetingId);
    void MeetingJoined(const QString &meetingId);
    void MeetingEnded();

private:
    Engine(QObject *parent = nullptr);
    ~Engine();
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    static Engine *m_instance;

    QString m_current_username;
    QString m_current_meetingId;
};

#endif // __ENGINE_H__
