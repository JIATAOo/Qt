#ifndef __JOIN_MEETING_DIALOG_H__
#define __JOIN_MEETING_DIALOG_H__

#include <QDialog>
#include <QString>

namespace Ui {
class JoinMeetingDialog;
}

class JoinMeetingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit JoinMeetingDialog(QWidget *parent = nullptr);
    ~JoinMeetingDialog();
    QString GetMeetingId() const { return m_meetingId; }

private slots:
    void onJoinClicked();

private:
    Ui::JoinMeetingDialog *ui;
    QString m_meetingId;
};

#endif // __JOIN_MEETING_DIALOG_H__
