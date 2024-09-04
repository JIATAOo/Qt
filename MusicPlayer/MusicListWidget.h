#ifndef __MUSICLISTWIDGET_H__
#define __MUSICLISTWIDGET_H__
#include <QListWidget>
class MusicList;
class Music;
class MusicListWidget :public QListWidget
{
	Q_OBJECT;
public:
	MusicListWidget(QWidget* parent);
	~MusicListWidget();
	void addMusicItem(const Music& music);
	void setIcon(QIcon icon);
	QIcon getIcon();
	using QListWidget::indexFromItem;
	// 当前歌曲列表（存储的是歌曲信息）
	MusicList* m_musicList;
private:
	QIcon m_icon;
};
#endif // !__MUSICLISTWIDGET_H__

