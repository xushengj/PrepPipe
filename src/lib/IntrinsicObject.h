#ifndef INTRINSICOBJECT_H
#define INTRINSICOBJECT_H

#include "src/lib/ObjectBase.h"
#include "src/lib/FileBackedObject.h"
#include "src/utils/XMLUtilities.h"

#include <QObject>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QIcon>

#include <vector>

class IntrinsicObject : public FileBackedObject
{
    Q_OBJECT
public:
    explicit IntrinsicObject(ObjectType ty);
    IntrinsicObject(const IntrinsicObject& src) = default;

    virtual ~IntrinsicObject() override {}

    static IntrinsicObject* loadFromXML(QXmlStreamReader &xml);

    void saveToXML(QXmlStreamWriter& xml);

    // provide a default implementation (just dump the xml)
    virtual QWidget* getEditor() override;
    virtual QWidget* getViewer() override;

    virtual bool saveToFile() override final;

    virtual QString getFileNameFilter() const override {
        return tr("PrepPipe XML Files (*.xml)");
    }

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter& xml) = 0;
};

// registration class (GUI only); used in IntrinsicObjectCreationDialog
class IntrinsicObjectDecl {
protected:
    IntrinsicObjectDecl();
    virtual ~IntrinsicObjectDecl() = default;

private:
    static std::vector<const IntrinsicObjectDecl*>* instancePtrVec;
public:
    static const std::vector<const IntrinsicObjectDecl*>& getInstancePtrVec();

public:
    // functions that derived class should implement

    // when using an ObjectType enum as category mark, remember that the enum should be *_START instead of *_END (enum value should be smaller than all children)
    virtual ObjectBase::ObjectType getObjectType() const = 0;

    // if this is a category mark, then this should be *_END
    virtual ObjectBase::ObjectType getChildMax() const {
        return getObjectType();
    }
    virtual QString getHTMLDocumentation() const {return IntrinsicObject::tr("Sorry, no documentation is available yet.");}

    // create the object and initialize it to default settings
    // if getChildMax() != getObjectType(), this function won't be called
    virtual IntrinsicObject* create(const QString& name) const = 0;
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
