#ifndef __PARTICIPANTS_PANEL_H__
#define __PARTICIPANTS_PANEL_H__

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QStringList>

class ParticipantsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ParticipantsPanel(QWidget *parent = nullptr);
    ~ParticipantsPanel();

    void setParticipants(const QStringList &participants);
    void addParticipant(const QString &name);
    void removeParticipant(const QString &name);
    int participantCount() const;

private:
    void setupUi();

    QLabel *m_titleLabel;
    QPushButton *m_closeBtn;
    QListWidget *m_participantList;
};

#endif // __PARTICIPANTS_PANEL_H__
