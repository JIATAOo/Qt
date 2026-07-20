#ifndef __CREATE_MEETING_DIALOG_H__
#define __CREATE_MEETING_DIALOG_H__

#include <QDialog>
#include <QString>

namespace Ui {
class CreateMeetingDialog;
}

class CreateMeetingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateMeetingDialog(QWidget *parent = nullptr);
    ~CreateMeetingDialog();
    QString GetMeetingId() const { return m_meetingId; }
    void SetMeetingId(const QString &id) { m_meetingId = id; UpdateIdLabel(); }

private:
    void UpdateIdLabel();

    Ui::CreateMeetingDialog *ui;
    QString m_meetingId;
};

#endif // __CREATE_MEETING_DIALOG_H__
