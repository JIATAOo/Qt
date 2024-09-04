#ifndef __K_GLOBAL_DATA__
#define __K_GLOBAL_DATA__

#include <QObject>

class KGlobalData : public QObject
{
	Q_OBJECT

public:
	enum class KDrawFlag
	{
		NoneDrawFlag = 0,
		MouseDrawFlag,	
		PenDrawFlag,
		LineDrawFlag,
		RectDrawFlag,
		CircleDrawFlag,
		TextDrawFlag
	};

	KGlobalData(QObject *parent = Q_NULLPTR);
	~KGlobalData();

	static KGlobalData *getGlobalDataIntance();

	void setDrawFlag(KGlobalData::KDrawFlag drawflag);
	KDrawFlag getDrawFlag();

	void setCanvasWidth(const int width);
	void setCanvasHeight(const int height);

	int getCanvasWidth() const;
	int getCanvasHeight() const;

	void setCanvasColor(const QString & colorStr);
	QString getCanvasColor();

	void setCanvaScale(qreal scale);
	qreal getCanvasScale();

private:
	KGlobalData(const KGlobalData &other) = delete;
	KGlobalData(const KGlobalData &&other) = delete;
	void operator=(const KGlobalData) = delete;

	KDrawFlag m_drawFlag;
	
	int m_canvasWidth;
	int m_canvasHeight;
	QString m_canvasColor;
	
	qreal m_scale;
};


#endif
