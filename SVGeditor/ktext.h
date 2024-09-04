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

    void drawShape(QPaintDevice* parent = nullptr) override;  // ��д�����ı��ķ���

    void setText(const QString& text);   // �����ı�
    QString getText() const;             // ��ȡ�ı�

    void setFont(const QFont& font);     // ��������
    void setColor(const QColor& color);  // �����ı���ɫ

    KShapeType getShapeType() override;  // ������״����

private:
    QString m_text;   // �ı�����
    QFont m_font;     // �ı�����
    QColor m_color;   // �ı���ɫ
};

#endif // KTEXT_H
