#ifndef __K_SHAPE_H_
#define __K_SHAPE_H_

#include <QObject>
#include <QPoint>
#include <QPaintDevice>
#include <QPainter>

enum class KShapeType
{
	None = 0,
	PenShapeType,
	LineShapeType,
	CircleShapeType,
	RectShapeType,
	PentShapeType,
	HexaShapeType,
	TextShapeType,
};

class KShape : public QObject
{
	Q_OBJECT

public:
	KShape(QObject *parent = Q_NULLPTR);
	~KShape();
	
	// ����������д��ʵ�ֲ�ͬ���͵�ͼ�εĻ���
	virtual void drawShape(QPaintDevice *parent = Q_NULLPTR) = 0;

	virtual void move(QPoint offset);
	virtual void moveTop(QPoint pos);
	virtual void moveBottom(QPoint pos);
	virtual void moveLeft(QPoint pos);
	virtual void moveRight(QPoint pos);
	virtual void moveTopLeft(QPoint pos);
	virtual void moveTopRight(QPoint pos);
	virtual void moveBottomLeft(QPoint pos);
	virtual void moveBottomRight(QPoint pos);
	virtual KShapeType getShapeType();
	

	QPoint getStartPoint();
	QPoint getEndPoint();

	void setStartPoint(const QPoint &point);
	void setEndPoint(const QPoint &point);
	

	void drawOutLine(QPaintDevice* parent = Q_NULLPTR);
	QRect getShapeRect() const;
	bool isValid();

protected:
	QPoint m_startPoint;// ��ʼ����
	QPoint m_endPoint; // ��������
};

#endif
