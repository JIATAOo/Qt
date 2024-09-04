#include "ktext.h"
#include <QPainter>

KText::KText(QObject* parent)
    : KShape(parent), m_text(""), m_font(QFont()), m_color(Qt::black)
{
}

KText::~KText()
{
}

void KText::drawShape(QPaintDevice* parent)
{
    if (parent == nullptr || m_text.isEmpty())
        return;

    QPainter painter(parent);
    painter.setFont(m_font);
    painter.setPen(m_color);

    // 在起始点绘制文本
    painter.drawText(m_startPoint, m_text);
}

void KText::setText(const QString& text)
{
    m_text = text;
}

QString KText::getText() const
{
    return m_text;
}

void KText::setFont(const QFont& font)
{
    m_font = font;
}

void KText::setColor(const QColor& color)
{
    m_color = color;
}

KShapeType KText::getShapeType()
{
    return KShapeType::TextShapeType;
}
