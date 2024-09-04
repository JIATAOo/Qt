#ifndef __MUSICLIST_H__
#define __MUSICLIST_H__
#include <QMediaPlaylist>
#include <QVector>
class Music;
#include <QVector>
#include "Music.h"

class MusicList
{
public:
    MusicList();
    void addMusic(const Music& music);
    const QVector<Music>* getMusicArray() const;

private:
    QVector<Music>* m_musicArray;
};
#endif // !__MUSICLIST_H__
