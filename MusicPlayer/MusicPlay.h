#ifndef __MUSICPLAY_H__
#define __MUSICPLAY_H__
#include <QObject>
class QMediaPlayer;
class QMediaPlaylist;
class MusicList;
class MusicPlay : public QObject
{
	Q_OBJECT;
public:
	MusicPlay(QObject* parent = nullptr);
	~MusicPlay();
	void initPlay();
	void playSong();
	void setPosition(int position);
	void onBtnPreClicked();
	void onBtnPlayClicked();
	void onBtnNextClicked();
	void updateCurrentMedia(int index);
	QMediaPlayer* getplayer();
	QMediaPlaylist* getplaylist();

private:
	QMediaPlayer* m_player;
	QMediaPlaylist* m_playlist;
};
#endif // !__MUSICPLAY_H__