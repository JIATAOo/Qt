#ifndef __K_CANVAS_H_
#define __K_CANVAS_H_

#include <QWidget>
#include <QPainterPath>
#include <QLineEdit>
#include "kshape.h"
#include "kglobaldata.h"

enum class KTransType
{
	None = 0,
	TopLeft, 
	Top,
	TopRight,
	Left,
	Contains,// ȫ�� 
	Right,
	BottomLeft,
	Bottom,
	BottomRight
};


class KCanvas : public QWidget
{
	Q_OBJECT

public:
	KCanvas(QWidget *parent = Q_NULLPTR);
	~KCanvas();

	virtual void paintEvent(QPaintEvent* event) override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	
	KShape* getCurrentShape(const QPoint &pos);;// ��ȡ��ǰλ��ͼ��
	KTransType getTransType(const QPoint &pos); // ��ȡ�ƶ�����
	void updateCusorStyle(KGlobalData::KDrawFlag flag, KTransType transType);
	void dragMoveShape(KTransType transType,const QPoint &pos);

private slots:
	void onTextInputFinished();  // �ı�������ɺ����
private:
	KShape* m_pCurrentShape;
	QList <KShape*> m_pShapeList;// �洢��ǰ�����е�ͼ��
	
	QPoint m_lastPos;// ��¼ǰһ�ε�λ��
	KTransType m_TransType;// ��¼�ƶ�����
	
	QPainterPath m_path;
	QLineEdit* m_textInput;      // �ı������
	QPoint m_textPos;
	bool m_isLPress;// ���������
	bool m_isDrawing;// �Ƿ��ͼ
	bool m_isSelected;// �Ƿ�Ϊѡ��
};

#endif
