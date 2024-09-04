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
	Contains,// 全部 
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
	
	KShape* getCurrentShape(const QPoint &pos);;// 获取当前位置图形
	KTransType getTransType(const QPoint &pos); // 获取移动类型
	void updateCusorStyle(KGlobalData::KDrawFlag flag, KTransType transType);
	void dragMoveShape(KTransType transType,const QPoint &pos);

private slots:
	void onTextInputFinished();  // 文本输入完成后调用
private:
	KShape* m_pCurrentShape;
	QList <KShape*> m_pShapeList;// 存储当前画布中的图形
	
	QPoint m_lastPos;// 记录前一次的位置
	KTransType m_TransType;// 记录移动类型
	
	QPainterPath m_path;
	QLineEdit* m_textInput;      // 文本输入框
	QPoint m_textPos;
	bool m_isLPress;// 标记鼠标左键
	bool m_isDrawing;// 是否绘图
	bool m_isSelected;// 是否为选中
};

#endif
