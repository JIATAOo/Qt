#include "kglobaldata.h"
#include <Windows.h>


KGlobalData::KGlobalData(QObject *parent)
	: QObject(parent)
	,m_drawFlag(KDrawFlag::NoneDrawFlag)
    ,m_canvasColor("FFFFFF")
    ,m_canvasWidth(600)
    ,m_canvasHeight(400)
    ,m_scale(1.0)
{
}



KGlobalData *KGlobalData::getGlobalDataIntance()
{
    static KGlobalData s_instance;
    static KGlobalData* pinstance = nullptr;
    if (pinstance == nullptr)
        pinstance = &s_instance;
    return pinstance;
}

void KGlobalData::setDrawFlag(KGlobalData::KDrawFlag drawflag)
{
    this->m_drawFlag = drawflag;
}

KGlobalData::KDrawFlag KGlobalData::getDrawFlag()
{
    return m_drawFlag;
}

void KGlobalData::setCanvasWidth(const int width)
{
    this->m_canvasWidth = width;
}

void KGlobalData::setCanvasHeight(const int height)
{
    this->m_canvasHeight = height;
}

int KGlobalData::getCanvasWidth() const
{
    return this->m_canvasWidth;
}

int KGlobalData::getCanvasHeight() const
{
    return m_canvasHeight;
}

void KGlobalData::setCanvasColor(const QString& colorStr)
{
    this->m_canvasColor = colorStr;
}

QString KGlobalData::getCanvasColor()
{
    return m_canvasColor;
}

void KGlobalData::setCanvaScale(qreal scale)
{
    m_scale = scale;
    
}

qreal KGlobalData::getCanvasScale()
{
    return m_scale;
}

KGlobalData::~KGlobalData()
{
}
