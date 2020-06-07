#include "SimpleTextGeneratorGUIObject.h"
#include "src/gui/SimpleTextGenerator/STGEditor.h"

SimpleTextGeneratorGUIObject::SimpleTextGeneratorGUIObject()
{

}

SimpleTextGeneratorGUIObject::SimpleTextGeneratorGUIObject(const SimpleTextGenerator::Data& dataArg, const QHash<QString, RuleGUIData>& guiDataArg)
    : SimpleTextGeneratorObject(dataArg), guidata(guiDataArg)
{}

QStringList SimpleTextGeneratorGUIObject::getHeaderExampleText(const QString& canonicalName) const {
    auto iter = guidata.find(canonicalName);
    if (iter != guidata.end()) {
        return iter.value().headerExampleText;
    }
    return QStringList();
};

QStringList SimpleTextGeneratorGUIObject::getDelimiterExampleText(const QString& canonicalName) const {
    auto iter = guidata.find(canonicalName);
    if (iter != guidata.end()) {
        return iter.value().delimiterExampleText;
    }
    return QStringList();
};

QStringList SimpleTextGeneratorGUIObject::getTailExampleText(const QString& canonicalName) const {
    auto iter = guidata.find(canonicalName);
    if (iter != guidata.end()) {
        return iter.value().tailExampleText;
    }
    return QStringList();
};

namespace {
const QString XML_HEADER_EXAMPLE_TEXT = QStringLiteral("HeaderParameterExampleText");
const QString XML_DELIMITER_EXAMPLE_TEXT = QStringLiteral("DelimiterParameterExampleText");
const QString XML_TAIL_EXAMPLE_TEXT = QStringLiteral("TailParameterExampleText");
const QString XML_EXAMPLE = QStringLiteral("Example");
const QString XML_EDITOR_GUI_DATA = QStringLiteral("EditorGUIData");
const QString XML_RULE_GUI_DATA = QStringLiteral("Rule");
const QString XML_CANONICAL_NAME = QStringLiteral("CanonicalName");
}
void SimpleTextGeneratorGUIObject::RuleGUIData::saveToXML(QXmlStreamWriter& xml) const
{
    XMLUtil::writeStringList(xml, headerExampleText,    XML_HEADER_EXAMPLE_TEXT,    XML_EXAMPLE, false);
    XMLUtil::writeStringList(xml, delimiterExampleText, XML_DELIMITER_EXAMPLE_TEXT, XML_EXAMPLE, false);
    XMLUtil::writeStringList(xml, tailExampleText,      XML_TAIL_EXAMPLE_TEXT,      XML_EXAMPLE, false);
    xml.writeEndElement();
}

bool SimpleTextGeneratorGUIObject::RuleGUIData::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleTextGeneratorGUIObject::RuleGUIData";
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_HEADER_EXAMPLE_TEXT,       XML_EXAMPLE, headerExampleText, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_DELIMITER_EXAMPLE_TEXT,    XML_EXAMPLE, delimiterExampleText, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_TAIL_EXAMPLE_TEXT,         XML_EXAMPLE, tailExampleText, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleTextGeneratorGUIObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    data.saveToXML_NoTerminate(xml);
    XMLUtil::writeLoadableHash(xml, guidata, XML_EDITOR_GUI_DATA, XML_RULE_GUI_DATA, XML_CANONICAL_NAME);
    xml.writeEndElement();
}

SimpleTextGeneratorGUIObject* SimpleTextGeneratorGUIObject::loadFromXML(QXmlStreamReader& xml, StringCache &strCache)
{
    SimpleTextGenerator::Data data;

    // we need to accept both the variant, with or without gui data
    if (Q_LIKELY(data.loadFromXML_NoTerminate(xml, strCache))){
        QHash<QString, RuleGUIData> guidata;
        const char* curElement = "SimpleTextGeneratorGUIObject";
        if (!XMLUtil::readLoadableHash(xml, curElement, XML_EDITOR_GUI_DATA, XML_RULE_GUI_DATA, XML_CANONICAL_NAME, guidata, strCache)) {
            guidata.clear();
        } else {
            xml.skipCurrentElement();
        }
        return new SimpleTextGeneratorGUIObject(data, guidata);
    }
    return nullptr;
}

QWidget* SimpleTextGeneratorGUIObject::getEditor()
{
    STGEditor* editor = new STGEditor();
    editor->setBackingObject(this);
    return editor;
}

// ----------------------------------------------------------------------------

class SimpleTextGeneratorDecl : public IntrinsicObjectDecl
{
public:
    SimpleTextGeneratorDecl() = default;
    virtual ~SimpleTextGeneratorDecl() = default;

    virtual ObjectBase::ObjectType getObjectType() const override {
        return ObjectBase::ObjectType::Task_SimpleTextGenerator;
    }

    virtual IntrinsicObject* create(const QString& name) const override {
        SimpleTextGeneratorGUIObject* obj = new SimpleTextGeneratorGUIObject;
        obj->setName(name);
        return obj;
    }
};
namespace {
SimpleTextGeneratorDecl objDecl;
}

