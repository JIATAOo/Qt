#ifndef __LOGIN_MAIN_WIDGET_H__
#define __LOGIN_MAIN_WIDGET_H__

#include <QDialog>
#include <QAction>

namespace Ui {
class LoginDlg;
}

class LoginDlg : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDlg(QWidget *parent = nullptr);
    ~LoginDlg();

    QString GetUserName() const { return m_user_name; }

private:
    void LoadConfig();
    void SetupPasswordToggle();
    QIcon CreateEyeIcon(bool visible) const;

private slots:
    void on_tbtn_new_register_clicked();
    void on_pbtn_register_clicked();
    void on_pbtn_cancel_clicked();
    void on_pbtn_login_clicked();
    void on_toggle_password_visibility();
    void on_login_success(const QString &username);
    void on_login_failed(const QString &reason);
    void on_register_success();
    void on_register_failed(const QString &reason);

private:
    Ui::LoginDlg *ui;
    QString m_user_name;
    QAction *m_pwd_toggle_action = nullptr;
    bool m_passwordVisible = false;
};

#endif // __LOGIN_MAIN_WIDGET_H__
