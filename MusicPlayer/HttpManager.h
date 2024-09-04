#ifndef __HTTPMANAGER_H__
#define __HTTPMANAGER_H__

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonArray>

class HttpManager : public QObject
{
    Q_OBJECT
public:
    explicit HttpManager(QObject* parent = nullptr);
    void fetchMusicJson(const QUrl& url);
    void downloadFile(const QUrl& url, const QString& type, QString fileName);
    QString m_baseUrl;
signals:
    void musicJsonFetched(const QJsonArray& list);
    void fileDownloaded(const QString& filePath, const QString& type);
    
private slots:
    void onDownloadJsonFinished();
    void onDownloadFileFinished();
    
private:
    QNetworkAccessManager m_networkManager;
};

#endif // HTTPMANAGER_H
