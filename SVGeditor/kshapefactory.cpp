#include "kshapefactory.h"
#include "krect.h"
#include "kline.h"
#include "kcircle.h"
#include "ktext.h"
#include "kpen.h"
KShapeFactory::KShapeFactory(QObject *parent)
	: QObject(parent)
{
}

KShapeFactory::~KShapeFactory()
{
}

KShape* KShapeFactory::createShape(KGlobalData::KDrawFlag drawFlag)
{
	switch(drawFlag)
	{
	case KGlobalData::KDrawFlag::RectDrawFlag:
		return new KRect;
	case KGlobalData::KDrawFlag::LineDrawFlag:
		return new KLine;
	case KGlobalData::KDrawFlag::CircleDrawFlag:
		return new KCircle;
	case KGlobalData::KDrawFlag::PenDrawFlag:
		return new KPen; 
	case KGlobalData::KDrawFlag::TextDrawFlag:
		return new KText;
		
	default:
		break;
	}

}
