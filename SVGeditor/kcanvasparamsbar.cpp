#include "kcanvasparamsbar.h"

KCanvasParamsBar::KCanvasParamsBar(const QString &title,QWidget *parent)
	: KParamsBar(title,parent)
	, m_pWidthBox(Q_NULLPTR)
	, m_pHeightBox(Q_NULLPTR)
	, m_pColorBox(Q_NULLPTR)
{
	m_pWidthBox = new KValueBox(QStringLiteral("���"), KGlobalData::getGlobalDataIntance()->getCanvasWidth(), this);
	m_pHeightBox = new KValueBox(QStringLiteral("�߶�"), KGlobalData::getGlobalDataIntance()->getCanvasHeight(), this);
	m_pColorBox = new KColorBox(QStringLiteral("��ɫ"), KGlobalData::getGlobalDataIntance()->getCanvasColor(), this);

	// ��ӵ����񲼾�
	m_pGridLayout->addWidget(m_pWidthBox, 0, 0);// �� 0 �� �� 0 ��
	m_pGridLayout->addWidget(m_pHeightBox, 0, 1);// �� 0 �� �� 1 ��
	m_pGridLayout->addWidget(m_pColorBox, 1, 0);

}

KCanvasParamsBar::~KCanvasParamsBar()
{
}
