#include "kcanvas.h"
#include "kshapefactory.h"
#include "kpen.h"
#include "ktext.h"
#include <QMouseEvent>
#include <QDebug>
KCanvas::KCanvas(QWidget* parent)
	: QWidget(parent)
	, m_pCurrentShape(Q_NULLPTR)
	, m_TransType(KTransType::None)
	, m_isDrawing(false)
	, m_isLPress(false)
	, m_isSelected(false)
{
	setAttribute(Qt::WA_StyledBackground, true);

	// 设置背景颜色为白色
	setStyleSheet("background-color:#FFFFFF");

	KGlobalData::getGlobalDataIntance()->setCanvasColor("FFFFFF");
	resize(KGlobalData::getGlobalDataIntance()->getCanvasWidth(),
		KGlobalData::getGlobalDataIntance()->getCanvasHeight());
	m_textInput = new QLineEdit(this);
	m_textInput->hide();  // 默认隐藏
	connect(m_textInput, &QLineEdit::editingFinished, this, &KCanvas::onTextInputFinished);

	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);
	

}

KCanvas::~KCanvas()
{
	for (auto shape : m_pShapeList)
		delete shape;
	m_pShapeList.clear();
}

void KCanvas::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制路径（对于 PenDrawFlag）
	if (KGlobalData::getGlobalDataIntance()->getDrawFlag() == KGlobalData::KDrawFlag::PenDrawFlag)
	{
		painter.setPen(QPen(Qt::black, 2));
		painter.drawPath(m_path);
	}

	// 绘制其他形状
	if (!m_pShapeList.isEmpty())
	{
		auto it = m_pShapeList.rbegin();
		while (it != m_pShapeList.rend())
		{
			(*it)->drawShape(this);
			++it;
		}
	}

	if (m_pCurrentShape != Q_NULLPTR)
	{
		if (m_isDrawing)
			m_pCurrentShape->drawShape(this);

		if (m_isSelected)
			m_pCurrentShape->drawOutLine(this);
	}
}


void KCanvas::mousePressEvent(QMouseEvent* event)
{
	KGlobalData::KDrawFlag flag = KGlobalData::getGlobalDataIntance()->getDrawFlag();

	if (flag == KGlobalData::KDrawFlag::NoneDrawFlag)
		return;

	if (event->buttons() == Qt::LeftButton)
	{
		m_isLPress = true;

		if (flag == KGlobalData::KDrawFlag::PenDrawFlag)
		{
			m_path.moveTo(event->pos()); // 记录路径起点
		}
		if (flag == KGlobalData::KDrawFlag::TextDrawFlag)
		{
			// 进入文本输入模式，显示输入框
			m_textPos = event->pos();  // 记录文本位置
			m_textInput->move(m_textPos);  // 移动文本框到鼠标点击位置
			m_textInput->setFixedSize(200, 30);  // 设置文本框大小
			m_textInput->show();
			m_textInput->setFocus();  // 让输入框获取焦点
		}
		else if (flag == KGlobalData::KDrawFlag::MouseDrawFlag)
		{
			m_pCurrentShape = getCurrentShape(event->pos());
			if (m_pCurrentShape != Q_NULLPTR)
			{
				m_lastPos = event->pos();
				m_isSelected = true;
				m_TransType = getTransType(event->pos());
			}
			else
			{
				m_isSelected = false;
			}
		}
		else
		{
			m_isSelected = false;
			m_pCurrentShape = KShapeFactory::createShape(flag);

			if (m_pCurrentShape != Q_NULLPTR)
				m_pCurrentShape->setStartPoint(event->pos());
		}
	}
	update();
}

void KCanvas::mouseMoveEvent(QMouseEvent* event)
{
	KGlobalData::KDrawFlag flag = KGlobalData::getGlobalDataIntance()->getDrawFlag();

	if (flag == KGlobalData::KDrawFlag::NoneDrawFlag)
		return;

	KTransType transType = getTransType(event->pos());
	updateCusorStyle(flag, transType);

	if (event->buttons() != Qt::LeftButton)
		return;

	if (flag == KGlobalData::KDrawFlag::PenDrawFlag)
	{
		m_path.lineTo(event->pos()); // 更新路径
		update(); // 请求重绘
	}
	else if (flag != KGlobalData::KDrawFlag::MouseDrawFlag)
	{
		if (m_isLPress && !m_isDrawing)
			m_isDrawing = true;

		if (m_pCurrentShape != Q_NULLPTR)
			m_pCurrentShape->setEndPoint(event->pos());
	}
	else
	{
		if (m_pCurrentShape == Q_NULLPTR)
			return;

		dragMoveShape(transType, event->pos());
	}
	update();
}

void KCanvas::mouseReleaseEvent(QMouseEvent* event)
{
	KGlobalData::KDrawFlag flag = KGlobalData::getGlobalDataIntance()->getDrawFlag();

	if (m_isLPress)
	{
		if (flag == KGlobalData::KDrawFlag::PenDrawFlag)
		{
			// 完成路径绘制
			if (!m_path.isEmpty())
			{
				m_pShapeList.prepend(new KPen(m_path)); // 使用 KPen 创建对象
				m_path = QPainterPath(); // 重置路径
			}
		}
		else if (flag != KGlobalData::KDrawFlag::MouseDrawFlag)
		{
			if (m_pCurrentShape != Q_NULLPTR)
			{
				if (m_pCurrentShape->isValid())
					m_pCurrentShape->setEndPoint(event->pos());

				m_pShapeList.prepend(m_pCurrentShape);
				m_pCurrentShape = Q_NULLPTR;
			}
			else
			{
				delete m_pCurrentShape;
				m_pCurrentShape = Q_NULLPTR;
			}
		}

		m_isLPress = false;
		m_isDrawing = false;
		m_TransType = KTransType::None;
	}

	update();
}



KShape* KCanvas::getCurrentShape(const QPoint &pos)
{
	for (QList<KShape*>::iterator it = m_pShapeList.begin();
		it != m_pShapeList.end(); ++it)
	{
		if ((*it)->getShapeRect().contains(pos))
			return *it;
	}

	return Q_NULLPTR;
}

KTransType KCanvas::getTransType(const QPoint& pos)
{
	if (!m_pCurrentShape)
		return KTransType::None;

	// 获取图形矩形
	QRect rect = m_pCurrentShape->getShapeRect();

	if (!rect.contains(pos))
		return KTransType::None;
	
	qDebug() << "rect = " << rect.topLeft().x();
	qDebug() << "rect = " << rect.topLeft().y();

	// 判断是整体移动，局部拖拽移动
	if (qAbs(pos.x() - rect.topLeft().x()) < 5 
			&& qAbs(pos.y() - rect.topLeft().y()) < 5)
		return KTransType::TopLeft;

	if (qAbs(rect.topRight().x() - pos.x()) < 5 
			&& qAbs(pos.y() - rect.topRight().y()) < 5)
		return KTransType::TopRight;

	if (qAbs(rect.bottomRight().x() - pos.x()) < 5 
			&& qAbs(rect.bottomRight().y() - pos.y()) < 5)
		return KTransType::BottomRight;

	if (qAbs(pos.x() - rect.bottomLeft().x()) < 5 
			&& qAbs(rect.bottomLeft().y() - pos.y()) < 5)
		return KTransType::BottomLeft;

	if (qAbs(pos.y() - rect.top()) < 5 
			&& pos.x() > rect.topLeft().x() + 5
			&& pos.x() < rect.topRight().x() - 5)
		return KTransType::Top;

	if (qAbs(rect.right() - pos.x()) < 5
			&& pos.y() > rect.topRight().y() + 5
			&& pos.y() < rect.bottomRight().y() - 5)
		return KTransType::Right;

	if (qAbs(rect.bottom() - pos.y()) < 5 
			&& pos.x() > rect.bottomLeft().x() + 5 
			&& pos.x() < rect.bottomRight().x() - 5)
		return KTransType::Bottom;

	if (qAbs(pos.x() - rect.left()) < 5 
			&& pos.y() > rect.topLeft().y() + 5 
			&& pos.y() < rect.bottomLeft().y() - 5)
		return KTransType::Left;

	return KTransType::Contains;
}

void KCanvas::updateCusorStyle(KGlobalData::KDrawFlag flag, KTransType transType)
{
	// 如果已经选中
	if (m_isSelected
		&& flag == KGlobalData::KDrawFlag::MouseDrawFlag
		&& transType != KTransType::None)
	{
		//TODO：变换图标,设置不同的鼠标样式,根据点击位置，设置不同的鼠标样式
		if (transType == KTransType::TopLeft || transType == KTransType::BottomRight)
			setCursor(Qt::SizeFDiagCursor);
		else if (transType == KTransType::TopRight || transType == KTransType::BottomLeft)
			setCursor(Qt::SizeBDiagCursor);
		else if (transType == KTransType::Top || transType == KTransType::Bottom)
			setCursor(Qt::SizeVerCursor);
		else if (transType == KTransType::Left || transType == KTransType::Right)
			setCursor(Qt::SizeHorCursor);

		else if (transType == KTransType::Contains)
			setCursor(Qt::SizeAllCursor);
	}
	else
		unsetCursor();
}

void KCanvas::dragMoveShape(KTransType transType,const QPoint &pos)
{
	if (m_pCurrentShape == NULL)
		return;

	/*
		在移动过程中，分为两种移动方式
			(1) 整体移动  contains
			(2) 基于某一个方向的拖拽移动

	*/
	switch (m_TransType)
	{
	case KTransType::None:
		return;
	case KTransType::TopLeft:
		// 如果是左上角，则当前位置为新的矩形起始位置
		m_pCurrentShape->moveTopLeft(pos);
		break;
	case KTransType::Top:
		// 如果是向上移动，则修改起始位置的 y 坐标
		m_pCurrentShape->moveTop(pos);
		break;
	case KTransType::TopRight:
		m_pCurrentShape->moveTopRight(pos);
		break;
	case KTransType::Left:
		m_pCurrentShape->moveLeft(pos);
		break;
	case KTransType::Contains: // 整体移动
	{
		// m_lastPos 为选中时光标位置，在移动过程中不断赋值为前一次的位置
		QPoint newpos = pos -  m_lastPos; // 计算要移动的偏移
		m_pCurrentShape->move(newpos);
		m_lastPos = pos;
	}
	break;
	case KTransType::Right:
		m_pCurrentShape->moveRight(pos);
		break;
	case KTransType::BottomLeft:
		m_pCurrentShape->moveBottomLeft(pos);
		break;
	case KTransType::Bottom:
		m_pCurrentShape->moveBottom(pos);
		break;
	case KTransType::BottomRight:
		m_pCurrentShape->moveBottomRight(pos);
		break;
	default:
		break;
	}
}
void KCanvas::onTextInputFinished()
{
	// 获取输入框中的文本
	QString text = m_textInput->text();
	m_textInput->hide();  // 隐藏输入框

	// 创建一个文本形状对象并添加到形状列表
	KText* textShape = new KText(this);
	textShape->setStartPoint(m_textPos);  // 设置文本的起始位置
	textShape->setText(text);             // 设置文本内容
	textShape->setFont(QFont("Arial", 16));  // 设置字体
	textShape->setColor(Qt::black);       // 设置颜色

	m_pShapeList.prepend(textShape);      // 将文本形状添加到形状列表

	update();  // 请求重绘，显示文本
}
