#include "SimpleTreeTransformObject.h"

SimpleTreeTransformObject::SimpleTreeTransformObject(const ConstructOptions &opt)
    : IntrinsicObject(ObjectType::SimpleTreeTransformObject, opt)
{

}

void SimpleTreeTransformObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    Q_UNUSED(xml)
}
