#include "kpen.h"
#include <QPainter>
KPen::KPen(QObject* parent)
    : KShape(parent)
    , m_path()
{}

KPen::KPen(const QPainterPath& path, QObject* parent)
    : KShape(parent)
    , m_path(path)
{}

void KPen::drawShape(QPaintDevice* parent)
{
    QPainter painter(parent);
    painter.drawPath(m_path); // »æÖÆÂ·¾¶
}
KShapeType KPen::getShapeType()
{
    return KShapeType::PenShapeType;
}
KPen::~KPen()
{
}