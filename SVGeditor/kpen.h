#ifndef KPEN_H
#define KPEN_H

#include <QPainterPath>
#include "kshape.h"

class KPen : public KShape
{
public:
    explicit KPen(QObject* parent = nullptr);
    explicit KPen(const QPainterPath& path, QObject* parent = nullptr);
    virtual void drawShape(QPaintDevice* parent = nullptr) override;
    virtual KShapeType getShapeType();
    ~KPen();
private:
    QPainterPath m_path; // ´æ´¢Â·¾¶Êý¾Ý
};

#endif // KPEN_H
