#include "krect.h"
#include <QPainter>

KRect::KRect(QObject *parent)
	: KShape(parent)
{
}

KRect::~KRect()
{
}

void KRect::drawShape(QPaintDevice* parent)
{
	QPainter painter(parent); 
	painter.drawRect(QRect(getStartPoint(),getEndPoint())); 
}

KShapeType KRect::getShapeType()
{
	return KShapeType::RectShapeType;
}
