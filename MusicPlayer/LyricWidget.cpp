#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMap>
#include <QDebug>
#include <QVBoxLayout>
#include "LyricWidget.h"
#include "ui_mainwindow.h"
#include "mainwindow.h"
LyricWidget::LyricWidget(QWidget* parent)
    : QWidget(parent)
{
	
}
LyricWidget::~LyricWidget()
{

}
bool LyricWidget::process(QString filePath)
{
	QFile lyricFile(filePath);
	lyricFile.open(QFile::ReadOnly);
	QString content(lyricFile.readAll());
	content.replace("\\r", "");
	content.replace("\\n", "");
	lyricFile.close();
	// 清空歌词
	m_lines.clear();
	const QRegExp rx("\\[(\\d+):(\\d+(\\.\\d+)?)\\]"); // 用来查找时间标签的正则表达式
	// 步骤1
	int pos = rx.indexIn(content);
	if (pos == -1)
	{
		return false;
	}
	else
	{
		int lastPos;
		QList<int> timeLabels;
		do
		{
			// 步骤2
			timeLabels << (rx.cap(1).toInt() * 60 + rx.cap(2).toDouble()) * 1000;
			lastPos = pos + rx.matchedLength();
			pos = rx.indexIn(content, lastPos);
			if (pos == -1)
			{
				QString text = content.mid(lastPos).trimmed();
				for (const int& time : timeLabels)
				{
					m_lines.push_back(LyricLine(time, text));
				}
				break;
			}
			// 步骤3
			QString text = content.mid(lastPos, pos - lastPos);
			if (!text.isEmpty())
			{
				for (const int& time : timeLabels)
				{
					m_lines.push_back(LyricLine(time, text.trimmed()));
				}
				timeLabels.clear();
			}
		} while (true);
		// 步骤4
		std::stable_sort(m_lines.begin(), m_lines.end());
	}
	if (m_lines.size())
		return true;
	return false;
}

int LyricWidget::getIndex(qint64 position)
{
	if (!m_lines.size())
	{
		return -1;
	}
	else
	{
		if (m_lines[0].time >= position)
			return 0;
	}
	int i = 1;
	for (i = 1; i < m_lines.size(); i++)
	{
		if (m_lines[i - 1].time < position && m_lines[i].time >= position)
			return i - 1;
	}
	return m_lines.size() - 1;
}
QString LyricWidget::getLyricText(int index)
{
	if (index < 0 || index >= m_lines.size())
		return "";
	return m_lines[index].text;
}
bool operator<(const LyricLine& A, const LyricLine& B)
{
	return A.time < B.time;
}
