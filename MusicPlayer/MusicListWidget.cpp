#include "MusicListWidget.h"
#include "MusicList.h"
#include "Music.h"
MusicListWidget::MusicListWidget(QWidget* parent)
	: QListWidget(parent)
	, m_musicList(new MusicList)
{
	
}

MusicListWidget::~MusicListWidget()
{
}

void MusicListWidget::addMusicItem(const Music& music)
{
	m_musicList->addMusic(music);
	addItem(music.getFilePath());
}

void MusicListWidget::setIcon(QIcon icon)
{
	m_icon = icon;
}

QIcon MusicListWidget::getIcon()
{
	return m_icon;
}