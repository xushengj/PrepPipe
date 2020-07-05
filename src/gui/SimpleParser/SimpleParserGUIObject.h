#ifndef SIMPLEPARSERGUIOBJECT_H
#define SIMPLEPARSERGUIOBJECT_H

#include <QObject>

#include "src/lib/TaskObject/SimpleParserObject.h"
#include "src/gui/SimpleParser/SimpleParserGUIExecuteObject.h"

class SimpleParserGUIObject : public SimpleParserObject
{
    Q_OBJECT
public:
    SimpleParserGUIObject();
    SimpleParserGUIObject(const SimpleParser::Data& data);
    SimpleParserGUIObject(const SimpleParserGUIObject&) = default;

    virtual SimpleParserGUIObject* clone() override {
        return new SimpleParserGUIObject(*this);
    }

    static SimpleParserGUIObject* loadFromXML(QXmlStreamReader& xml, StringCache &strCache);

    virtual QWidget* getEditor() override;

    virtual SimpleParserGUIExecuteObject* getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const override;

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;
};

template <>
struct IntrinsicObjectTrait<SimpleParserObject>
{
    static SimpleParserObject* loadFromXML(QXmlStreamReader& xml, StringCache& strCache) {
        return SimpleParserGUIObject::loadFromXML(xml, strCache);
    }
};

#endif // SIMPLEPARSERGUIOBJECT_H
