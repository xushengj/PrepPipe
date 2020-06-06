#ifndef INTRINSICOBJECT_H
#define INTRINSICOBJECT_H

#include "src/lib/ObjectBase.h"
#include "src/lib/FileBackedObject.h"
#include "src/utils/XMLUtilities.h"

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

    virtual bool saveToFile() override final;

    virtual QString getFileNameFilter() const override {
        return tr("PrepPipe XML Files (*.xml)");
    }

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter& xml) = 0;
};

// if an intrinsic type implements editor GUI,
// specialize this template to diverge xml load call to pick up editor data from xml
template <typename IntrinsicTy>
struct IntrinsicObjectTrait
{
    static IntrinsicTy* loadFromXML(QXmlStreamReader& xml, StringCache& strCache) {
        return IntrinsicTy::loadFromXML(xml, strCache);
    }
};

#endif // INTRINSICOBJECT_H
