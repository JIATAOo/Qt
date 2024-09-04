#ifndef _MAINWINDOW_H__
#define _MAINWINDOW_H__

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QSystemTrayIcon>
#include <QNetworkReply>
#include <QGraphicsPixmapItem>
#include "ui_mainwindow.h"
#include "LyricWidget.h"
#include "HttpManager.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MusicPlay;
class LyricsShow;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void initUiStyle();
    void initPlay();
    void setConnect();
    void changePlayButton();
    void onBtnAddClicked();
    void onBtnVolumeClicked();
    void onVolumeSliderValueChanged(int value);
    void setPosition(int position);
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void updateInfo();
    void showLyric(qint64 position);
    void clearLyric();
    void onPlayListWidgetDoubleClicked(QListWidgetItem* item);
    QString formatTime(qint64 timeMilliSeconds);
    void onBtndownloadClicked();
    void onMusicJsonFetched(const QJsonArray& list);
    void onFileDownloaded(const QString& filePath, const QString& type);
    void onBtnDownloadClicked();
    void updatePlaylist(const QString& filePath, const QString& fileName);
    void onDownloadListWidgetDoubleClicked(QListWidgetItem* item);
    void scanLocalMusicDirectory(const QString& directoryPath);
    void rotateCoverLabel();
    void on_minButton_clicked();
    void on_exitButton_clicked();
    QImage loadImage(const QString& author, const QString& title);
    void updatePlaylistIcons();
    void loadLyrics(const QString& author, const QString& title);
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
private:  
    Ui::MainWindow* ui;
    MusicPlay* m_musicPlay;
    QSystemTrayIcon* m_systemTray;
    QNetworkAccessManager* m_networkManager;
    QUrl m_url;
    QString m_baseUrl;
    int previousVolume = 0;
    LyricWidget* lyricWidget;
    QString m_saveMusicPath;
    QString m_saveLyricPath;
    QString m_saveImgPath;
    HttpManager* m_httpManager;
    QMap<QString, QString> m_urlMap;
    QJsonArray musicListArray; 
    qreal m_rotationAngle  = 0;
    QPoint m_dragPosition;
    bool m_dragging;
    QPixmap originalPixmap;
    QPixmap centerPixmap;
    QTimer* angleTimer;
    double angle = 0.0;
    enum PlayMode {
        Sequential,
        Random,
        RepeatOne
    };
    PlayMode currentPlayMode;
    void updatePlayMode();
    void on_playModeComboBox_currentIndexChanged(int index);
};

#endif // _MAINWINDOW_H__