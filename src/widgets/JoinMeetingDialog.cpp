#include "JoinMeetingDialog.h"
#include "ui_JoinMeetingDialog.h"
#include <QDebug>

JoinMeetingDialog::JoinMeetingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::JoinMeetingDialog)
{
    ui->setupUi(this);

    connect(ui->idEdit, &QLineEdit::returnPressed, this, &JoinMeetingDialog::onJoinClicked);
    connect(ui->joinBtn, &QPushButton::clicked, this, &JoinMeetingDialog::onJoinClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

JoinMeetingDialog::~JoinMeetingDialog()
{
    delete ui;
}

void JoinMeetingDialog::onJoinClicked()
{
    m_meetingId = ui->idEdit->text().trimmed();
    if (m_meetingId.isEmpty())
    {
        ui->errorLabel->setText(tr("Please enter a meeting ID"));
        return;
    }
    qDebug() << "Joining meeting:" << m_meetingId;
    accept();
}
