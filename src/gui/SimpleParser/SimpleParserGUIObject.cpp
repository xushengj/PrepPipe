#include "SimpleParserGUIObject.h"
#include "src/gui/SimpleParser/SimpleParserEditor.h"

SimpleParserGUIObject::SimpleParserGUIObject()
{

}

SimpleParserGUIObject::SimpleParserGUIObject(const SimpleParser::Data& data)
    : SimpleParserObject(data)
{

}

SimpleParserGUIObject* SimpleParserGUIObject::loadFromXML(QXmlStreamReader& xml, StringCache &strCache)
{
    SimpleParser::Data data;

    // we need to accept both the variant, with or without gui data
    if (Q_LIKELY(data.loadFromXML_NoTerminate(xml, strCache))){
        // we currently do not have any gui only data yet
        xml.skipCurrentElement();
        return new SimpleParserGUIObject(data);
    }

    return nullptr;
}

void SimpleParserGUIObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    data.saveToXML_NoTerminate(xml);
    // currently there is no other data to save
    xml.writeEndElement();
}

QWidget* SimpleParserGUIObject::getEditor()
{
    SimpleParserEditor* editor = new SimpleParserEditor;
    editor->setBackingObject(this);
    return editor;
}
