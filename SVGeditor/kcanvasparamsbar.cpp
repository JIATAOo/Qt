#include "kcanvasparamsbar.h"

KCanvasParamsBar::KCanvasParamsBar(const QString &title,QWidget *parent)
	: KParamsBar(title,parent)
	, m_pWidthBox(Q_NULLPTR)
	, m_pHeightBox(Q_NULLPTR)
	, m_pColorBox(Q_NULLPTR)
{
	m_pWidthBox = new KValueBox(QStringLiteral("宽度"), KGlobalData::getGlobalDataIntance()->getCanvasWidth(), this);
	m_pHeightBox = new KValueBox(QStringLiteral("高度"), KGlobalData::getGlobalDataIntance()->getCanvasHeight(), this);
	m_pColorBox = new KColorBox(QStringLiteral("颜色"), KGlobalData::getGlobalDataIntance()->getCanvasColor(), this);

	// 添加到网格布局
	m_pGridLayout->addWidget(m_pWidthBox, 0, 0);// 第 0 行 第 0 列
	m_pGridLayout->addWidget(m_pHeightBox, 0, 1);// 第 0 行 第 1 列
	m_pGridLayout->addWidget(m_pColorBox, 1, 0);

}

KCanvasParamsBar::~KCanvasParamsBar()
{
}
