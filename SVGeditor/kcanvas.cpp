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

	// ���ñ�����ɫΪ��ɫ
	setStyleSheet("background-color:#FFFFFF");

	KGlobalData::getGlobalDataIntance()->setCanvasColor("FFFFFF");
	resize(KGlobalData::getGlobalDataIntance()->getCanvasWidth(),
		KGlobalData::getGlobalDataIntance()->getCanvasHeight());
	m_textInput = new QLineEdit(this);
	m_textInput->hide();  // Ĭ������
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

	// ����·�������� PenDrawFlag��
	if (KGlobalData::getGlobalDataIntance()->getDrawFlag() == KGlobalData::KDrawFlag::PenDrawFlag)
	{
		painter.setPen(QPen(Qt::black, 2));
		painter.drawPath(m_path);
	}

	// ����������״
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
			m_path.moveTo(event->pos()); // ��¼·�����
		}
		if (flag == KGlobalData::KDrawFlag::TextDrawFlag)
		{
			// �����ı�����ģʽ����ʾ�����
			m_textPos = event->pos();  // ��¼�ı�λ��
			m_textInput->move(m_textPos);  // �ƶ��ı��������λ��
			m_textInput->setFixedSize(200, 30);  // �����ı����С
			m_textInput->show();
			m_textInput->setFocus();  // ��������ȡ����
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
		m_path.lineTo(event->pos()); // ����·��
		update(); // �����ػ�
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
			// ���·������
			if (!m_path.isEmpty())
			{
				m_pShapeList.prepend(new KPen(m_path)); // ʹ�� KPen ��������
				m_path = QPainterPath(); // ����·��
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

	// ��ȡͼ�ξ���
	QRect rect = m_pCurrentShape->getShapeRect();

	if (!rect.contains(pos))
		return KTransType::None;
	
	qDebug() << "rect = " << rect.topLeft().x();
	qDebug() << "rect = " << rect.topLeft().y();

	// �ж��������ƶ����ֲ���ק�ƶ�
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
	// ����Ѿ�ѡ��
	if (m_isSelected
		&& flag == KGlobalData::KDrawFlag::MouseDrawFlag
		&& transType != KTransType::None)
	{
		//TODO���任ͼ��,���ò�ͬ�������ʽ,���ݵ��λ�ã����ò�ͬ�������ʽ
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
		���ƶ������У���Ϊ�����ƶ���ʽ
			(1) �����ƶ�  contains
			(2) ����ĳһ���������ק�ƶ�

	*/
	switch (m_TransType)
	{
	case KTransType::None:
		return;
	case KTransType::TopLeft:
		// ��������Ͻǣ���ǰλ��Ϊ�µľ�����ʼλ��
		m_pCurrentShape->moveTopLeft(pos);
		break;
	case KTransType::Top:
		// ����������ƶ������޸���ʼλ�õ� y ����
		m_pCurrentShape->moveTop(pos);
		break;
	case KTransType::TopRight:
		m_pCurrentShape->moveTopRight(pos);
		break;
	case KTransType::Left:
		m_pCurrentShape->moveLeft(pos);
		break;
	case KTransType::Contains: // �����ƶ�
	{
		// m_lastPos Ϊѡ��ʱ���λ�ã����ƶ������в��ϸ�ֵΪǰһ�ε�λ��
		QPoint newpos = pos -  m_lastPos; // ����Ҫ�ƶ���ƫ��
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
	// ��ȡ������е��ı�
	QString text = m_textInput->text();
	m_textInput->hide();  // ���������

	// ����һ���ı���״������ӵ���״�б�
	KText* textShape = new KText(this);
	textShape->setStartPoint(m_textPos);  // �����ı�����ʼλ��
	textShape->setText(text);             // �����ı�����
	textShape->setFont(QFont("Arial", 16));  // ��������
	textShape->setColor(Qt::black);       // ������ɫ

	m_pShapeList.prepend(textShape);      // ���ı���״��ӵ���״�б�

	update();  // �����ػ棬��ʾ�ı�
}
