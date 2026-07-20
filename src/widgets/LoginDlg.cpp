#include "LoginDlg.h"
#include "ui_LoginDlg.h"
#include "Engine.h"
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

LoginDlg::LoginDlg(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDlg)
{
    ui->setupUi(this);
    setFixedSize(350, 525);
    setWindowTitle(tr("Login"));

    ui->stackedWidget->setCurrentWidget(ui->login_wgt);

    SetupPasswordToggle();
    LoadConfig();

    connect(ui->tbtn_new_register, &QToolButton::clicked, this, &LoginDlg::on_tbtn_new_register_clicked);
    connect(ui->pbtn_register, &QPushButton::clicked, this, &LoginDlg::on_pbtn_register_clicked);
    connect(ui->pbtn_cancel, &QPushButton::clicked, this, &LoginDlg::on_pbtn_cancel_clicked);
    connect(ui->pbtn_login, &QPushButton::clicked, this, &LoginDlg::on_pbtn_login_clicked);

    connect(Engine::Instance(), &Engine::LoginSuccess, this, &LoginDlg::on_login_success);
    connect(Engine::Instance(), &Engine::LoginFailed, this, &LoginDlg::on_login_failed);
    connect(Engine::Instance(), &Engine::RegisterSuccess, this, &LoginDlg::on_register_success);
    connect(Engine::Instance(), &Engine::RegisterFailed, this, &LoginDlg::on_register_failed);
}

LoginDlg::~LoginDlg()
{
    delete ui;
}

void LoginDlg::LoadConfig()
{
    QString username, password;
    if (Engine::Instance()->LoadSavedCredentials(username, password))
    {
        ui->ledt_user_name->setText(username);
        ui->ledt_user_password->setText(password);
        ui->ccbox_rember_password->setChecked(true);
    }
}

void LoginDlg::SetupPasswordToggle()
{
    m_pwd_toggle_action = ui->ledt_user_password->addAction(
        CreateEyeIcon(false), QLineEdit::TrailingPosition);
    m_pwd_toggle_action->setToolTip(tr("Show password"));
    connect(m_pwd_toggle_action, &QAction::triggered, this, &LoginDlg::on_toggle_password_visibility);
}

QIcon LoginDlg::CreateEyeIcon(bool visible) const
{
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#ffffff"), 1.5));
    painter.setBrush(Qt::NoBrush);

    painter.drawEllipse(QPointF(12, 12), 7, 7);
    painter.drawEllipse(QPointF(12, 12), 3, 3);

    if (visible)
    {
        painter.drawLine(QPointF(3, 3), QPointF(21, 21));
    }

    painter.end();
    return QIcon(pixmap);
}

void LoginDlg::on_toggle_password_visibility()
{
    m_passwordVisible = !m_passwordVisible;
    if (m_passwordVisible)
    {
        ui->ledt_user_password->setEchoMode(QLineEdit::Normal);
        m_pwd_toggle_action->setIcon(CreateEyeIcon(true));
        m_pwd_toggle_action->setToolTip(tr("Hide password"));
    }
    else
    {
        ui->ledt_user_password->setEchoMode(QLineEdit::Password);
        m_pwd_toggle_action->setIcon(CreateEyeIcon(false));
        m_pwd_toggle_action->setToolTip(tr("Show password"));
    }
}

void LoginDlg::on_tbtn_new_register_clicked()
{
    qDebug() << "jump to register page";
    setWindowTitle(tr("Register"));
    ui->lab_error_info->clear();
    ui->stackedWidget->setCurrentWidget(ui->register_wgt);
}

void LoginDlg::on_pbtn_register_clicked()
{
    qDebug() << "enter register process";

    auto name = ui->ledt_user_name->text();
    auto pwd = ui->ledt_user_password->text();
    if (name.isEmpty() || pwd.isEmpty())
    {
        ui->lab_error_info->setText(tr("name or password is empty"));
        return;
    }

    Engine::Instance()->Register(name, pwd);
}

void LoginDlg::on_register_success()
{
    ui->stackedWidget->setCurrentWidget(ui->login_wgt);
    setWindowTitle(tr("Login"));
    ui->lab_error_info->clear();
    qDebug() << "register success, switch to login page";
}

void LoginDlg::on_register_failed(const QString &reason)
{
    ui->lab_error_info->setText(reason);
    qDebug() << "register failed:" << reason;
}

void LoginDlg::on_pbtn_cancel_clicked()
{
    qDebug() << "cancel register, switch to login page";
    ui->stackedWidget->setCurrentWidget(ui->login_wgt);
    setWindowTitle(tr("Login"));
    ui->lab_error_info->clear();
}

void LoginDlg::on_pbtn_login_clicked()
{
    qDebug() << "enter login process";
    auto name = ui->ledt_user_name->text();
    auto pwd = ui->ledt_user_password->text();
    if (name.isEmpty() || pwd.isEmpty())
    {
        ui->lab_error_info->setText(tr("name or password is empty"));
        return;
    }

    if (ui->ccbox_rember_password->isChecked())
    {
        Engine::Instance()->SaveCredentials(name, pwd);
    }

    Engine::Instance()->Login(name, pwd);
}

void LoginDlg::on_login_success(const QString &username)
{
    m_user_name = username;
    qDebug() << "login success, user:" << username;
    done(QDialog::Accepted);
}

void LoginDlg::on_login_failed(const QString &reason)
{
    ui->lab_error_info->setText(reason);
    qDebug() << "login failed:" << reason;
}
