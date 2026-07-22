#ifndef __CHAT_PANEL_H__
#define __CHAT_PANEL_H__

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QString>

class ChatPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPanel(const QString &username, QWidget *parent = nullptr);
    ~ChatPanel();

signals:
    void messageSent(const QString &username, const QString &message);

public slots:
    void addMessage(const QString &sender, const QString &message);

private slots:
    void onSendClicked();

private:
    void setupUi();

    QString m_username;

    QLabel *m_titleLabel;
    QPushButton *m_closeBtn;
    QListWidget *m_messageList;
    QLineEdit *m_inputEdit;
    QPushButton *m_sendBtn;
};

#endif // __CHAT_PANEL_H__
