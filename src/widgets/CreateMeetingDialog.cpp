#include "CreateMeetingDialog.h"
#include "ui_CreateMeetingDialog.h"
#include <QClipboard>
#include <QApplication>
#include <QDebug>

CreateMeetingDialog::CreateMeetingDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CreateMeetingDialog)
{
    ui->setupUi(this);

    connect(ui->copyBtn, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(m_meetingId);
        qDebug() << "Meeting ID copied:" << m_meetingId;
    });

    connect(ui->closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

CreateMeetingDialog::~CreateMeetingDialog()
{
    delete ui;
}

void CreateMeetingDialog::UpdateIdLabel()
{
    ui->idLabel->setText(tr("Meeting ID: %1").arg(m_meetingId));
}
