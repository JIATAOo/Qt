#include <QApplication>
#include <QFile>
#include <QStandardPaths>

#include "Logger.h"
#include "LoginDlg.h"
#include "MainWindow.h"

int main(int argc, char* argv[])
{
    // 初始化日志：控制台 + 文件（AppData/logs/）
    Logger::Logger::instance().initialize(
        Logger::Output::Both,
        Logger::Level::Debug,
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation).toStdString() + "/logs");

    QApplication app(argc, argv);

    // 加载全局样式表
    QFile qss_file(":/qss/default/LoginDlg.qss");
    if (qss_file.open(QFile::ReadOnly))
    {
        app.setStyleSheet(qss_file.readAll());
        qss_file.close();
    }
    else
    {
        LOG_ERROR("Failed to load LoginDlg.qss");
    }

    LoginDlg login;
    if (login.exec() != QDialog::Accepted)
    {
        return 0;
    }

    LOG_INFO_S << "Login successful, user: " << login.GetUserName().toStdString();

    MainWindow mainWindow(login.GetUserName());
    mainWindow.show();

    return app.exec();
}
