#include "kline.h"
#include <QPainter>

KLine::KLine(QObject* parent)
	: KShape(parent)
{
}

KLine::~KLine()
{
}

void KLine::drawShape(QPaintDevice* parent)
{
	QPainter painter(parent);
	painter.drawLine(getShapeRect().topLeft(), getShapeRect().bottomRight());
}

KShapeType KLine::getShapeType()
{
	return KShapeType::LineShapeType;
}
