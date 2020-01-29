#ifndef SIMPLETREETRANSFORMOBJECT_H
#define SIMPLETREETRANSFORMOBJECT_H

#include "src/lib/IntrinsicObject.h"

#include <QObject>

class SimpleTreeTransformObject : public IntrinsicObject
{
    Q_OBJECT
public:

    SimpleTreeTransformObject(const ConstructOptions& opt);

    static SimpleTreeTransformObject* loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt);

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

private:

};

#endif // SIMPLETREETRANSFORMOBJECT_H
