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
    IntrinsicObject(const IntrinsicObject& src) = default;

    virtual ~IntrinsicObject() override {}

    static IntrinsicObject* loadFromXML(QXmlStreamReader &xml, const ConstructOptions& opt);

    void saveToXML(QXmlStreamWriter& xml);

    virtual QWidget* getEditor() override;

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter& xml) = 0;
};

#endif // INTRINSICOBJECT_H
