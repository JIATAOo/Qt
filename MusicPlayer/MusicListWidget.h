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
	// ��ǰ�����б��洢���Ǹ�����Ϣ��
	MusicList* m_musicList;
private:
	QIcon m_icon;
};
#endif // !__MUSICLISTWIDGET_H__

