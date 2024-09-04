#ifndef __MUSIC_H__
#define __MUSIC_H__

#include <QString>

class Music
{
public:
    Music(const QString& filePath);

    QString getFilePath() const;
    QString getLyricFile() const;

private:
    QString m_filePath;
    QString m_lyricFile;
};

#endif // __MUSIC_H__
