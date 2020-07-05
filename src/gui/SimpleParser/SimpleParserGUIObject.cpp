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

SimpleParserGUIExecuteObject* SimpleParserGUIObject::getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const
{
    Q_UNUSED(options)
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)
    return new class SimpleParserGUIExecuteObject(data, getName());
}

// ----------------------------------------------------------------------------

class SimpleParserDecl : public IntrinsicObjectDecl
{
public:
    SimpleParserDecl() = default;
    virtual ~SimpleParserDecl() = default;

    virtual ObjectBase::ObjectType getObjectType() const override {
        return ObjectBase::ObjectType::Task_SimpleParser;
    }

    virtual IntrinsicObject* create(const QString& name) const override {
        // create the initial (default) data
        QString rootName = SimpleParserGUIObject::tr("root");
        SimpleParser::Data data;
        data.rootRuleNodeName = rootName;
        data.whitespaceList.push_back(QStringLiteral(" "));
        data.whitespaceList.push_back(QStringLiteral("\t"));
        SimpleParser::MatchRuleNode rootRule;
        rootRule.name = rootName;
        data.matchRuleNodes.push_back(rootRule);
        SimpleParser::MatchRuleNode exampleRule;
        exampleRule.name = SimpleParserGUIObject::tr("Example");
        exampleRule.parentNodeNameList.push_back(rootName);
        data.matchRuleNodes.push_back(exampleRule);
        data.flag_skipEmptyLineBeforeMatching = true;

        SimpleParserGUIObject* obj = new SimpleParserGUIObject(data);
        obj->setName(name);
        return obj;
    }
};
namespace {
SimpleParserDecl objDecl;
}
