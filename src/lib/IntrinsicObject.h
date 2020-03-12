#ifndef INTRINSICOBJECT_H
#define INTRINSICOBJECT_H

#include "src/lib/ObjectBase.h"
#include "src/lib/FileBackedObject.h"

#include <QObject>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class IntrinsicObject : public FileBackedObject
{
    Q_OBJECT
public:
    explicit IntrinsicObject(ObjectType ty);
    IntrinsicObject(const IntrinsicObject& src) = default;

    virtual ~IntrinsicObject() override {}

    static IntrinsicObject* loadFromXML(QXmlStreamReader &xml);

    void saveToXML(QXmlStreamWriter& xml);

    virtual QWidget* getEditor() override;

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter& xml) = 0;
};

#endif // INTRINSICOBJECT_H
