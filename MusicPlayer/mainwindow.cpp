#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "MusicPlay.h"
#include "LyricWidget.h"
#include "MusicListWidget.h"
#include "MusicList.h"
#include "Music.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUrl>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QPainter>
#include <QVBoxLayout>
#include <QTimer>
#include <QPixmap>
#include <QTransform>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_musicPlay(new MusicPlay(this))
	, m_networkManager(new QNetworkAccessManager(this))
	, m_systemTray(new QSystemTrayIcon(this)) 
	, lyricWidget(new LyricWidget(this))
	, m_httpManager(new HttpManager(this))
	, m_rotationAngle(0)
	, m_dragging(false)
	, angleTimer(new QTimer(this))
{
    ui->setupUi(this);
    initUiStyle();
    initPlay();
    setConnect(); 
	scanLocalMusicDirectory(QDir::currentPath() + "/Music/");
}

MainWindow::~MainWindow()
{
    delete ui;
	delete m_musicPlay;
}
void MainWindow::initUiStyle()
{
	this->setStyleSheet(
		"QMainWindow {"
		"    background-image: url(:/image/background.png);"
		"    background-position: center;"
		"    background-repeat: repeat;"
		"    background-size: auto;"
		"}"
	);
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
	ui->addFileButton->setIcon(QIcon(":/image/add-line.png"));
	ui->lastButton->setIcon(QIcon(":/image/previous.png"));
	ui->nextButton->setIcon(QIcon(":/image/next.png"));
	ui->playButton->setIcon(QIcon(":/image/play.png"));
	ui->volume->setIcon(QIcon(":/image/sound.png"));
	ui->downloadFileButton->setIcon(QIcon(":/image/search.png"));
	ui->minButton->setIcon(QIcon(":/image/-.png"));
	ui->exitButton->setIcon(QIcon(":/image/x.png"));
	ui->coverLabel->setFixedSize(200, 200);
	ui->playModeComboBox->addItem(QString::fromLocal8Bit("列表循环"));
	ui->playModeComboBox->addItem(QString::fromLocal8Bit("随机播放"));
	ui->playModeComboBox->addItem(QString::fromLocal8Bit("单曲循环"));
	QPixmap pixmap(":/image/player.png");
	pixmap = pixmap.scaled(ui->coverLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
	QPixmap roundPixmap(ui->coverLabel->size());
	roundPixmap.fill(Qt::transparent);
	QPainter painter(&roundPixmap);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::white);
	painter.drawEllipse(roundPixmap.rect());
	painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
	painter.drawPixmap(0, 0, pixmap);
	ui->coverLabel->setPixmap(roundPixmap);
}

void MainWindow::initPlay()
{
	m_musicPlay->initPlay();
	connect(ui->songProgress, &QAbstractSlider::valueChanged, this, &MainWindow::setPosition);
	connect(m_musicPlay->getplayer(), &QMediaPlayer::positionChanged, this, &MainWindow::updatePosition);
	connect(m_musicPlay->getplayer(), &QMediaPlayer::durationChanged, this, &MainWindow::updateDuration);
	connect(m_musicPlay->getplayer(), &QMediaPlayer::metaDataAvailableChanged, this, &MainWindow::updateInfo);
}
void MainWindow::setConnect()
{
	connect(ui->lastButton, &QPushButton::clicked, m_musicPlay, &MusicPlay::onBtnPreClicked);
	connect(ui->playButton, &QPushButton::clicked, m_musicPlay, &MusicPlay::onBtnPlayClicked);
	connect(m_musicPlay->getplayer(), &QMediaPlayer::stateChanged, this, &MainWindow::changePlayButton);
	connect(ui->nextButton, &QPushButton::clicked, m_musicPlay, &MusicPlay::onBtnNextClicked);
	connect(ui->addFileButton, &QPushButton::clicked, this, &MainWindow::onBtnAddClicked);
	connect(ui->downloadFileButton, &QPushButton::clicked, this, &MainWindow::onBtndownloadClicked);
	connect(ui->volume, &QPushButton::clicked, this, &MainWindow::onBtnVolumeClicked);
	connect(ui->volumeLevel, &QSlider::valueChanged, this, &MainWindow::onVolumeSliderValueChanged);
	connect(ui->musicListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlayListWidgetDoubleClicked);
	connect(m_httpManager, &HttpManager::musicJsonFetched, this, &MainWindow::onMusicJsonFetched);
	connect(m_httpManager, &HttpManager::fileDownloaded, this, &MainWindow::onFileDownloaded);
	connect(ui->downloadListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onDownloadListWidgetDoubleClicked);
	connect(angleTimer, &QTimer::timeout, this, &MainWindow::rotateCoverLabel);
	connect(ui->minButton, &QPushButton::clicked, this, &MainWindow::on_minButton_clicked);
	connect(ui->exitButton, &QPushButton::clicked, this, &MainWindow::on_exitButton_clicked);
	connect(ui->playModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),this, &MainWindow::on_playModeComboBox_currentIndexChanged);
}
void MainWindow::changePlayButton()
{
	if (m_musicPlay->getplayer()->state() == QMediaPlayer::PlayingState)
	{
		ui->playButton->setIcon(QIcon(":/image/pause.png"));
		angleTimer->start(50);
	}
	else
	{
		ui->playButton->setIcon(QIcon(":/image/play.png"));
		angleTimer->stop();
	}
}
void MainWindow::onBtnAddClicked()
{
	
	QFileDialog fileDialog(this);
	fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
	fileDialog.setFileMode(QFileDialog::ExistingFiles);
	fileDialog.setWindowTitle(QStringLiteral("添加本地音乐"));
	// 设置文件后缀过滤器
	QStringList nameFilters;
	nameFilters << QStringLiteral("音频文件 (*.mp3 *.flac *.mpga)")
		<< QStringLiteral("MP3 文件 (*.mp3)")
		<< QStringLiteral("FLAC 文件 (*.flac)")
		<< QStringLiteral("MPGA 文件 (*.mpga)");
	fileDialog.setNameFilters(nameFilters);
	fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MusicLocation).value(0, QDir::homePath()));
	if (fileDialog.exec() == QDialog::Accepted)
	{
		QList<QUrl> urls = fileDialog.selectedUrls();
		for (const QUrl& url : urls)
		{
			// 获取文件路径并添加到 MusicListWidget
			QString filePath = url.toLocalFile();
			QString fileName = QFileInfo(filePath).fileName();
			qDebug() << "Adding file:" << filePath;
			updatePlaylist(filePath, fileName);
		}
	}
}

void MainWindow::onBtndownloadClicked()
{
	ui->downloadListWidget->clear();
	QUrl url;
	QString s = ui->lineEditLink->text().trimmed();
	if (s == QStringLiteral("孤勇者"))
		url = QUrl("https://gitee.com/MarkYangUp/music/raw/master/%E5%AD%A4%E5%8B%87%E8%80%85/music.json");
	else if (s == QStringLiteral("风雨无阻"))
		url = QUrl("https://gitee.com/MarkYangUp/music/raw/master/%E9%A3%8E%E9%9B%A8%E6%97%A0%E9%98%BB/music.json");
	else
	{
		QMessageBox::information(NULL,
			QString::fromLocal8Bit("提示"),
			QString::fromLocal8Bit("抱歉, 歌曲待更新"),
			QMessageBox::Yes);
	}
	if (url.isValid()) //"https://gitee.com/MarkYangUp/music/raw/master/%E9%A3%8E%E9%9B%A8%E6%97%A0%E9%98%BB/music.json"
	{
		m_httpManager->fetchMusicJson(url);
	}
	else {
		qDebug() << "Invalid URL";
	}
}
void MainWindow::onBtnVolumeClicked()
{
	int currentVolume = m_musicPlay->getplayer()->volume();
	if (currentVolume != 0)
	{
		previousVolume = currentVolume; 
		m_musicPlay->getplayer()->setVolume(0);
		ui->volumeLevel->setValue(0);
	}
	else
	{
		m_musicPlay->getplayer()->setVolume(previousVolume);
		
		ui->volumeLevel->setValue(previousVolume);
	}
}
void MainWindow::onVolumeSliderValueChanged(int value)
{
	m_musicPlay->getplayer()->setVolume(value);
	if(value != 0)
		previousVolume = value;
}
void MainWindow::setPosition(int position)
{
	if (qAbs(m_musicPlay->getplayer()->position() - position) > 99)
		m_musicPlay->getplayer()->setPosition(position);
}
void MainWindow::updatePosition(qint64 position)
{
	ui->songProgress->setValue(static_cast<int>(position));
	ui->positionLabel->setText(formatTime(position) + "/" + formatTime(m_musicPlay->getplayer()->duration()));
	if (m_musicPlay->getplaylist()->currentIndex() >= 0)
		showLyric(position);
}
void MainWindow::updateDuration(qint64 duration)
{
	ui->songProgress->setRange(0, static_cast<int>(duration));
	ui->songProgress->setEnabled(static_cast<int>(duration) > 0);
	if (!(static_cast<int>(duration) > 0))
	{
		// 无音乐播放时，界面元素
		ui->infoLabel->setText("KEEP CALM AND CARRY ON ...");
		m_systemTray->setToolTip(QStringLiteral("音乐播放器"));
		QImage image(":/image/player.png");
		ui->coverLabel->setPixmap(QPixmap::fromImage(image));
		ui->TitleLabel->setText("");
		ui->AlbumLabel->setText("");
		ui->AuthorLabel->setText("");
		clearLyric();
	}
	ui->songProgress->setPageStep(static_cast<int>(duration) / 10);
}
void MainWindow::updateInfo()
{
	if (m_musicPlay->getplayer()->isMetaDataAvailable())
	{
		// 返回可用MP3元数据列表（调试时可以查看）
		QStringList listInfo_debug = m_musicPlay->getplayer()->availableMetaData();
		// 歌曲信息
		QString info = "";
		QString author = m_musicPlay->getplayer()->metaData(QStringLiteral("Author")).toStringList().join(",");
		QString title = m_musicPlay->getplayer()->metaData(QStringLiteral("Title")).toString();
		QString albumTitle = m_musicPlay->getplayer()->metaData(QStringLiteral("AlbumTitle")).toString();

		info.append(author);
		info.append(" - " + title);
		info.append(" [" + formatTime(m_musicPlay->getplayer()->duration()) + "]");
		ui->infoLabel->setText(info);
		m_systemTray->setToolTip("正在播放：" + info);

		// 获取封面图片
		QImage picImage;
		QString baseFilePath = QDir::currentPath() + "/img/" + author + " - " + title ;
		qDebug() << baseFilePath;
		QString imagePath = baseFilePath + ".png";
		if (QFile::exists(imagePath))
		{
			picImage.load(imagePath);
		}
		else
		{
			picImage = m_musicPlay->getplayer()->metaData(QStringLiteral("ThumbnailImage")).value<QImage>();
		}
		if (picImage.isNull())
			picImage.load(m_saveImgPath);
		if (picImage.isNull())
			 picImage = QImage(":/image/player.png");
		ui->coverLabel->setPixmap(QPixmap::fromImage(picImage));
		ui->coverLabel->setScaledContents(true);
		
		QPixmap pixmap(imagePath);
		pixmap = pixmap.scaled(ui->coverLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		QPixmap roundPixmap(ui->coverLabel->size());
		roundPixmap.fill(Qt::transparent);
		QPainter painter(&roundPixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(Qt::NoPen);
		painter.setBrush(Qt::white);
		painter.drawEllipse(roundPixmap.rect());
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.drawPixmap(0, 0, pixmap);
		originalPixmap = roundPixmap;
		ui->coverLabel->setPixmap(originalPixmap);
		centerPixmap.load(imagePath);
		// 改变正在播放歌曲的图标
		for (int i = 0; i < m_musicPlay->getplaylist()->mediaCount(); i++)
		{
			QListWidgetItem* p = ui->musicListWidget->item(i);
			p->setIcon(ui->musicListWidget->getIcon());
		}
		int index = m_musicPlay->getplaylist()->currentIndex();
		QListWidgetItem* p = ui->musicListWidget->item(index);
		p->setIcon(QIcon("imagePath"));

		// 歌词界面显示的信息
		ui->TitleLabel->setText(title);
		ui->AlbumLabel->setText(QStringLiteral("专辑：") + albumTitle);
		ui->AuthorLabel->setText(QStringLiteral("歌手：") + author);

		QString lyricFilePath = QDir::currentPath() + "/lyric/" + author + " - " + title + ".txt";
		if (QFile::exists(lyricFilePath))
		{
			ui->lyricWidget->process(lyricFilePath);
		}
		else
		{
			ui->lyricWidget->process("lyricFilePath");
		}
	}
}

QString MainWindow::formatTime(qint64 timeMilliSeconds)
{
	qint64 seconds = timeMilliSeconds / 1000;
	const qint64 minutes = seconds / 60;
	seconds -= minutes * 60;
	return QStringLiteral("%1:%2")
		.arg(minutes, 2, 10, QLatin1Char('0'))
		.arg(seconds, 2, 10, QLatin1Char('0'));
}
void MainWindow::showLyric(qint64 position)
{
	int index = ui->lyricWidget->getIndex(position);
	if (index == -1)
	{
		ui->label->setText("");
		ui->label_2->setText("");
		ui->label_3->setText("");
		ui->label_4->setText(QStringLiteral("当前歌曲无歌词"));
		ui->label_5->setText("");
		ui->label_6->setText("");
		ui->label_7->setText("");
	}
	else
	{
		ui->label->setText(ui->lyricWidget->getLyricText(index - 3));
		ui->label_2->setText(ui->lyricWidget->getLyricText(index - 2));
		ui->label_3->setText(ui->lyricWidget->getLyricText(index - 1));
		ui->label_4->setText(ui->lyricWidget->getLyricText(index));
		ui->label_5->setText(ui->lyricWidget->getLyricText(index + 1));
		ui->label_6->setText(ui->lyricWidget->getLyricText(index + 2));
		ui->label_7->setText(ui->lyricWidget->getLyricText(index + 3));
	}
}

void MainWindow::clearLyric()
{
	ui->label->setText("");
	ui->label_2->setText("");
	ui->label_3->setText("");
	ui->label_4->setText("");
	ui->label_5->setText("");
	ui->label_6->setText("");
	ui->label_7->setText("");
}
void MainWindow::onPlayListWidgetDoubleClicked(QListWidgetItem* item)
{
	if (!item) 
	{
		qWarning() << "Null item double-clicked!";
		return;
	}

	QModelIndex index = ui->musicListWidget->indexFromItem(item);
	if (!index.isValid()) 
	{
		qWarning() << "Invalid index for item!";
		return;
	}

	int i = index.row();
	qDebug() << "Double clicked item at row:" << i;

	if (i < 0 || i >= m_musicPlay->getplaylist()->mediaCount())
	{
		qWarning() << "Index out of bounds!";
		return;
	}

	m_musicPlay->getplaylist()->setCurrentIndex(i);
	m_musicPlay->getplayer()->play();

	qDebug() << "Player state:" << m_musicPlay->getplayer()->state();
}

void MainWindow::onMusicJsonFetched(const QJsonArray& list)
{
	musicListArray = list;  // 存储音乐对象列表

	for (const QJsonValue& value : list)
	{
		QJsonObject musicObj = value.toObject();
		QString songFile = musicObj["mp3"].toString();
		QString lyricFile = musicObj["lyric"].toString();
		QString albumImgFile = musicObj["img"].toString();
		QString musicName = musicObj["musicName"].toString();
		QString albumName = musicObj["albumName"].toString();
		int duration = musicObj["duration"].toInt();
		QString path = musicObj["path"].toString();

		int firstSlashIndex = path.indexOf('/');
		int secondSlashIndex = path.indexOf('/', firstSlashIndex + 1);

		
		if (firstSlashIndex != -1 && secondSlashIndex != -1 && secondSlashIndex > firstSlashIndex) 
		{
			path.remove(firstSlashIndex + 1, secondSlashIndex - firstSlashIndex );
		}
		qDebug() << path;
		// 构建完整的 URL
		QString fullPath = m_httpManager->m_baseUrl;
		qDebug() << fullPath;
		if (!fullPath.endsWith("/"))
			fullPath += "/";
		if (path.startsWith("/"))
			path.remove(0, 1);
		QString p = 
		fullPath += path;
		if (!fullPath.endsWith("/"))
			fullPath += "/";
		
		qDebug() << "Song " << songFile;
		qDebug() << "Lyric " << lyricFile;
		qDebug() << "Album Image" << albumImgFile;

		QString songUrlString = fullPath + songFile;
		QString lyricUrlString = fullPath + lyricFile;
		QString albumImgUrlString = fullPath + albumImgFile;

		qDebug() << "Song URL:" << songUrlString;
		qDebug() << "Lyric URL:" << lyricUrlString;
		qDebug() << "Album Image URL:" << albumImgUrlString;
		// 将 URL 和类型存储到 m_urlMap
		m_urlMap[songUrlString] = "song";
		m_urlMap[lyricUrlString] = "lyric";
		m_urlMap[albumImgUrlString] = "albumImg";

		// 将音乐名称添加到 downloadListWidget 中
		
		QListWidgetItem* item = new QListWidgetItem(musicName, ui->downloadListWidget);
		ui->downloadListWidget->addItem(item);
		
	}
}



void MainWindow::onBtnDownloadClicked()
{
	QListWidgetItem* item = ui->downloadListWidget->currentItem();
	if (!item)
	{
		qWarning() << "No item selected!";
		return;
	}

	QModelIndex index = ui->downloadListWidget->indexFromItem(item);
	if (!index.isValid())
	{
		qWarning() << "Invalid index for item!";
		return;
	}

	int i = index.row();
	qDebug() << "Selected item at row:" << i;

	if (i < 0 || i >= musicListArray.size())
	{
		qWarning() << "Index out of bounds!";
		return;
	}

	QJsonObject musicObj = musicListArray[i].toObject();
	QString songFile = musicObj["mp3"].toString();
	QString lyricFile = musicObj["lyric"].toString();
	QString albumImgFile = musicObj["img"].toString();
	QString musicName = musicObj["musicName"].toString();
	QString artistName = musicObj["artistName"].toString();
	QString path = musicObj["path"].toString();

	// 构建完整的 URL
	QString fullPath = m_httpManager->m_baseUrl;
	if (!fullPath.endsWith("/"))
		fullPath += "/";
	if (path.startsWith("/"))
		path.remove(0, 1);
	fullPath += path;
	if (!fullPath.endsWith("/"))
		fullPath += "/";

	QString songUrlString = fullPath + songFile;
	QString lyricUrlString = fullPath + lyricFile;
	QString albumImgUrlString = fullPath + albumImgFile;
	QString songFileName = musicName + " - " + artistName + ".mp3";
	QString lyricFileName = musicName + " - " + artistName + ".txt";
	QString albumImgFileName = musicName + " - " + artistName + ".jpg";

	// 下载歌曲文件
	if (!songUrlString.isEmpty() && m_urlMap.contains(songUrlString))
	{
		m_httpManager->downloadFile(QUrl(songUrlString), m_urlMap[songUrlString], songFileName);
	}

	// 下载歌词文件
	if (!lyricUrlString.isEmpty() && m_urlMap.contains(lyricUrlString))
	{
		m_httpManager->downloadFile(QUrl(lyricUrlString), m_urlMap[lyricUrlString], lyricFileName);
	}

	// 下载专辑图片文件
	if (!albumImgUrlString.isEmpty() && m_urlMap.contains(albumImgUrlString))
	{
		m_httpManager->downloadFile(QUrl(albumImgUrlString), m_urlMap[albumImgUrlString], albumImgFileName);
	}
}



void MainWindow::onDownloadListWidgetDoubleClicked(QListWidgetItem* item)
{
	if (!item)
	{
		qWarning() << "Null item double-clicked!";
		return;
	}

	QModelIndex index = ui->downloadListWidget->indexFromItem(item);
	if (!index.isValid())
	{
		qWarning() << "Invalid index for item!";
		return;
	}

	int i = index.row();
	qDebug() << "Double clicked item at row:" << i;

	if (i < 0 || i >= musicListArray.size())
	{
		qWarning() << "Index out of bounds!";
		return;
	}

	QJsonObject musicObj = musicListArray[i].toObject();

	QString path = musicObj["path"].toString();
	int firstSlashIndex = path.indexOf('/');
	int secondSlashIndex = path.indexOf('/', firstSlashIndex + 1);


	if (firstSlashIndex != -1 && secondSlashIndex != -1 && secondSlashIndex > firstSlashIndex)
	{
		path.remove(firstSlashIndex + 1, secondSlashIndex - firstSlashIndex);
	}
	QString basePath = m_httpManager->m_baseUrl;
	
	QString fullPath = basePath + path;
	if (!fullPath.endsWith("/"))
		fullPath += "/";
	QString musicName = musicObj["musicName"].toString();
	QString songUrlString = fullPath + musicObj["mp3"].toString();
	QString lyricUrlString = fullPath + musicObj["lyric"].toString();
	QString albumImgUrlString = fullPath + musicObj["img"].toString();
	qDebug() << songUrlString;
	qDebug() << lyricUrlString ;
	qDebug() << albumImgUrlString;
	// 下载歌曲文件
	if (!songUrlString.isEmpty())
	{
		m_httpManager->downloadFile(QUrl(songUrlString), "song", musicName);
	}

	// 下载歌词文件
	if (!lyricUrlString.isEmpty())
	{
		m_httpManager->downloadFile(QUrl(lyricUrlString), "lyric", musicName);
	}

	// 下载专辑图片文件
	if (!albumImgUrlString.isEmpty())
	{
		m_httpManager->downloadFile(QUrl(albumImgUrlString), "albumImg", musicName);
	}
}


void MainWindow::onFileDownloaded(const QString& filePath, const QString& type)
{
	qDebug() << "File downloaded: " << filePath << ", type: " << type;

	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists()) {
		qWarning() << "File does not exist: " << filePath;
		return;
	}

	if (type == "song")
	{
		QString fileName = fileInfo.fileName();  // 仅获取文件名
		updatePlaylist(filePath, fileName);
	}
	else if (type == "lyric")
	{
		lyricWidget->process(filePath);
	}
	else if (type == "albumImg")
	{
		QPixmap albumCover(filePath);
		if (!albumCover.isNull())
		{
			ui->coverLabel->setPixmap(albumCover);
			ui->coverLabel->setScaledContents(true);
		}
	}
	if (type == "song")
	QMessageBox::information(this, tr("Download Complete"), tr("File downloaded and saved to: %1").arg(filePath));
}

void MainWindow::updatePlaylist(const QString& filePath, const QString& fileName)
{
	// 添加文件名到音乐列表小部件
	QListWidgetItem* item = new QListWidgetItem(fileName, ui->musicListWidget);
	ui->musicListWidget->addItem(item);

	// 添加媒体到播放列表
	m_musicPlay->getplaylist()->addMedia(QUrl::fromLocalFile(filePath));
}
void MainWindow::scanLocalMusicDirectory(const QString& directoryPath)
{
	QDir dir(directoryPath);
	if (!dir.exists())
	{
		qWarning() << "Directory does not exist: " << directoryPath;
		return;
	}

	QStringList nameFilters;
	nameFilters << "*.mp3" << "*.flac" << "*.mpga";
	QFileInfoList fileList = dir.entryInfoList(nameFilters, QDir::Files);

	for (const QFileInfo& fileInfo : fileList)
	{
		QString filePath = fileInfo.absoluteFilePath();
		QString fileName = fileInfo.fileName();
		qDebug() << "Found local music file:" << filePath;

		// 添加到播放列表
		updatePlaylist(filePath, fileName);
	}
}
void MainWindow::rotateCoverLabel()
{
		angle += 0.5;
		if (angle >= 360.0)
		{
			angle = 0.0;
		}

		QPixmap rotatedPixmap(originalPixmap.size());
		rotatedPixmap.fill(Qt::transparent); 

		QPainter painter(&rotatedPixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		
		QTransform transform;
		transform.translate(rotatedPixmap.width() / 2, rotatedPixmap.height() / 2);
		transform.rotate(angle);
		transform.translate(-rotatedPixmap.width() / 2, -rotatedPixmap.height() / 2);

		painter.setTransform(transform);
		painter.drawPixmap(0, 0, originalPixmap); 

		
		QPixmap rotatedCenterPixmap(rotatedPixmap.size());
		rotatedCenterPixmap.fill(Qt::transparent); 

		QPainter centerPainter(&rotatedCenterPixmap);
		centerPainter.setRenderHint(QPainter::Antialiasing, true);
		centerPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		
		centerPainter.setTransform(transform);

		
		auto centerImgRadius = rotatedPixmap.width() / 3.2; 
		QRect centerImgRect((rotatedPixmap.width() - centerImgRadius) / 2,
			(rotatedPixmap.height() - centerImgRadius) / 2,
			centerImgRadius, centerImgRadius);

		QRegion maskedRegion(centerImgRect, QRegion::Ellipse);
		centerPainter.setClipRegion(maskedRegion);

		
		centerPainter.drawPixmap(centerImgRect, centerPixmap);

		
		QPainter finalPainter(&rotatedPixmap);
		finalPainter.drawPixmap(0, 0, rotatedCenterPixmap);

		ui->coverLabel->setPixmap(rotatedPixmap);
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_dragPosition = event->globalPos() - frameGeometry().topLeft();
		m_dragging = true;
		event->accept();
	}
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
	if (m_dragging && (event->buttons() & Qt::LeftButton))
	{
		move(event->globalPos() - m_dragPosition);
		event->accept();
	}
}
void MainWindow::on_minButton_clicked()
{
	// Minimize the window
	this->showMinimized();
}

void MainWindow::on_exitButton_clicked()
{
	// Exit the application
	this->close();
}
void MainWindow::on_playModeComboBox_currentIndexChanged(int index)
{
	switch (index) {
	case 0:
		currentPlayMode = Sequential;
		break;
	case 1:
		currentPlayMode = Random;
		break;
	case 2:
		currentPlayMode = RepeatOne;
		break;
	default:
		currentPlayMode = Sequential;
		break;
	}

	updatePlayMode();
}

void MainWindow::updatePlayMode()
{

	switch (currentPlayMode) {
	case Sequential:
		m_musicPlay->getplaylist()->setPlaybackMode(QMediaPlaylist::Loop);
		break;
	case Random:
		m_musicPlay->getplaylist()->setPlaybackMode(QMediaPlaylist::Random); 
		break;
	case RepeatOne:
		m_musicPlay->getplaylist()->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
		break;
	}
}