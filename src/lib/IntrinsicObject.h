#ifndef INTRINSICOBJECT_H
#define INTRINSICOBJECT_H

#include "src/lib/ObjectBase.h"

#include <QObject>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class IntrinsicObject : public ObjectBase
{
    Q_OBJECT
public:
    IntrinsicObject(ObjectType ty, const ConstructOptions& opt);
    virtual ~IntrinsicObject() {}

    static IntrinsicObject* loadFromXML(QXmlStreamReader &xml, const ConstructOptions& opt);

    void saveToXML(QXmlStreamWriter& xml);

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter& xml) = 0;
};

#endif // INTRINSICOBJECT_H
