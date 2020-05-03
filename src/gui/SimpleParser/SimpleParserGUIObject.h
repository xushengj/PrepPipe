#ifndef SIMPLEPARSERGUIOBJECT_H
#define SIMPLEPARSERGUIOBJECT_H

#include <QObject>

#include "src/lib/TaskObject/SimpleParserObject.h"

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
