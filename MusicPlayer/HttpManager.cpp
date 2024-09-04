#include "HttpManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

HttpManager::HttpManager(QObject* parent) : QObject(parent)
{
    
}

void HttpManager::fetchMusicJson(const QUrl& url)
{
    m_baseUrl = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toString();
    qDebug() << "Base URL:" << m_baseUrl;

    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(onDownloadJsonFinished()));
}


void HttpManager::downloadFile(const QUrl& url,const QString& type, QString filename)
{
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager.get(request);
    reply->setProperty("fileType", type);
    reply->setProperty("fileName", filename);
    connect(reply, SIGNAL(finished()), this, SLOT(onDownloadFileFinished()));
}

void HttpManager::onDownloadJsonFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
    {
        qDebug() << "Error: sender is not a QNetworkReply";
        return;
    }

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error downloading JSON:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    qDebug() << "JSON Response size:" << responseData.size();
    qDebug() << "JSON Response data:" << responseData;

    if (responseData.isEmpty())
    {
        qDebug() << "Downloaded JSON is empty!";
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject())
    {
        qDebug() << "Invalid JSON format";
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QJsonArray musicArray = jsonObj["list"].toArray();
    qDebug() << "Music array:" << musicArray;
    emit musicJsonFetched(musicArray);

    reply->deleteLater();

}
void HttpManager::onDownloadFileFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply)
    {
        qDebug() << "Error: sender is not a QNetworkReply";
        return;
    }

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error downloading file:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray fileData = reply->readAll();
    qDebug() << "Downloaded file size:" << fileData.size();

    if (fileData.isEmpty())
    {
        qDebug() << "Downloaded file is empty!";
        reply->deleteLater();
        return;
    }

    QString fileType = reply->property("fileType").toString();

    QString fileName = reply->property("fileName").toString();
    QString downloadDirectory;

    // 根据文件类型设置下载目录
    if (fileType == "song") 
    {
        downloadDirectory = QDir::currentPath() + "/Music";
        fileName += ".mp3";
    }
    else if (fileType == "lyric") 
    {
        downloadDirectory = QDir::currentPath() + "/lyric";
        fileName += ".txt";
    }
    else if (fileType == "albumImg") 
    {
        downloadDirectory = QDir::currentPath() + "/img";
        fileName += ".png";
    }

    // 创建目录（如果不存在）
    QDir dir;
    if (!dir.exists(downloadDirectory)) 
    {
        if (!dir.mkpath(downloadDirectory))
        {
            qDebug() << "Error creating directory:" << downloadDirectory;
            reply->deleteLater();
            return;
        }
    }
    QString filePath = downloadDirectory + "/" + fileName;
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Error opening file for writing:" << filePath;
        reply->deleteLater();
        return;
    }

    if (file.write(fileData) == -1)
    {
        qDebug() << "Error writing file:" << filePath;
        file.close();
        reply->deleteLater();
        return;
    }

    file.close();
    qDebug() << "Downloaded and saved file:" << filePath;

    emit fileDownloaded(filePath, fileType ); 

    reply->deleteLater();
}

