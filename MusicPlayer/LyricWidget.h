#ifndef __LYRICSSHOW_H__
#define __LYRICSSHOW_H__
#include <QWidget>
#include <QLabel>
#include <QMap>

class LyricLine
{
public:
    LyricLine() :time(0), text("") {}
    LyricLine(qint64 time, QString text) :time(time), text(text) {}
    qint64 time = 0;
    QString text;
};
bool operator <(const LyricLine& A, const LyricLine& B);
class LyricWidget : public QWidget
{
    Q_OBJECT
public:
    LyricWidget(QWidget* parent = nullptr);
    ~LyricWidget();
    int getIndex(qint64 position);
    bool process(QString filePath);
    QString getLyricText(int index);
    
private:
    QVector<LyricLine> m_lines;
};
#endif // !__LYRICSSHOW_H__

