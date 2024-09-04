#ifndef KTEXT_H
#define KTEXT_H

#include "kshape.h"
#include <QString>
#include <QFont>
#include <QColor>

class KText : public KShape
{
public:
    KText(QObject* parent = nullptr);
    ~KText();

    void drawShape(QPaintDevice* parent = nullptr) override;  // 重写绘制文本的方法

    void setText(const QString& text);   // 设置文本
    QString getText() const;             // 获取文本

    void setFont(const QFont& font);     // 设置字体
    void setColor(const QColor& color);  // 设置文本颜色

    KShapeType getShapeType() override;  // 返回形状类型

private:
    QString m_text;   // 文本内容
    QFont m_font;     // 文本字体
    QColor m_color;   // 文本颜色
};

#endif // KTEXT_H
