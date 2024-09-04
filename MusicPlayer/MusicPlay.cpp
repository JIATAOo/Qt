#include "MusicPlay.h"
#include "MusicListWidget.h"
#include <QString>
#include <QStandardPaths>
#include <QFileDialog>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaPlayList>
#include <QObject>
#include <QWidget>
#include <QtMultimedia/QMediaContent>
#include <QUrl>
#include <qdebug.h>
#include <QModelIndex>
MusicPlay::MusicPlay(QObject* parent)
    :QObject(parent)
{
    m_player = new QMediaPlayer;
    m_playlist = new QMediaPlaylist;
    m_playlist->setPlaybackMode(QMediaPlaylist::Loop);
    m_player->setPlaylist(m_playlist);
}
MusicPlay::~MusicPlay()
{
    delete m_player;
    delete m_playlist;
}
void MusicPlay::initPlay()
{
    m_player->setVolume(50);
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &MusicPlay::updateCurrentMedia);
}

void MusicPlay::playSong()
{

}
 void MusicPlay::setPosition(int position)
{
    if (qAbs(m_player->position() - position) > 99)
        m_player->setPosition(position);
}

 QMediaPlayer* MusicPlay::getplayer()
 {
     return m_player;
 }

 QMediaPlaylist* MusicPlay::getplaylist()
 {
     return m_playlist;
 }
 void MusicPlay::onBtnPreClicked()
 {
     m_playlist->previous();
 }

 void MusicPlay::onBtnPlayClicked()
 {
     if (m_player->state() == QMediaPlayer::PlayingState)
     {
         m_player->pause();
     }
     else if (m_player->state() == QMediaPlayer::PausedState)
     {
         m_player->play();
     }
     else if (!m_playlist->isEmpty() && (m_player->state() == QMediaPlayer::StoppedState))
     {
         m_playlist->setCurrentIndex(0);
         m_player->play();
     }
 }

 void MusicPlay::onBtnNextClicked()
 {
     m_playlist->next();
 }

 void MusicPlay::updateCurrentMedia(int index)
 {
     if (index >= 0 && index < m_playlist->mediaCount())
     {
         qDebug() << "Current media changed to index: " << index;
     }
 }
