#include "Music.h"
#include <QRegExp>

Music::Music(const QString& filePath)
    : m_filePath(filePath)
{
    m_lyricFile = filePath;
    m_lyricFile.replace(QRegExp("\\.mp3$|\\.flac$|\\.mpga$"), ".lrc");
}

QString Music::getFilePath() const
{
    return m_filePath;
}

QString Music::getLyricFile() const
{
    return m_lyricFile;
}
