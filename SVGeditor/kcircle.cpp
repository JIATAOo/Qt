#include "kcircle.h"
#include <QPainter>

KCircle::KCircle(QObject* parent)
	: KShape(parent)
{
}

KCircle::~KCircle()
{
}

void KCircle::drawShape(QPaintDevice* parent)
{
	QPainter painter(parent);
	painter.drawEllipse(getShapeRect());
}

KShapeType KCircle::getShapeType()
{
	return KShapeType::CircleShapeType;
}
