#include "MusicList.h"
#include <QProgressDialog>
#include <QMimeDatabase>
#include "Music.h"
MusicList::MusicList()
    :m_musicArray(new QVector<Music>)
{

}
void MusicList::addMusic(const Music& music)
{
    m_musicArray->push_back(music);
}

const QVector<Music>* MusicList::getMusicArray() const
{
    return m_musicArray;
}
