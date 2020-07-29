#include "SimpleParserObject.h"
#include "src/lib/DataObject/PlainTextObject.h"
#include "src/lib/DataObject/GeneralTreeObject.h"

SimpleParserObject::SimpleParserObject()
    : TaskObject(ObjectType::Task_SimpleParser)
{

}

SimpleParserObject::SimpleParserObject(const SimpleParser::Data& data)
    : TaskObject(ObjectType::Task_SimpleParser), data(data)
{

}

SimpleParserObject* SimpleParserObject::loadFromXML(QXmlStreamReader& xml, StringCache &strCache)
{
    SimpleParser::Data data;
    if (Q_LIKELY(data.loadFromXML(xml, strCache))) {
        return new SimpleParserObject(data);
    }
    return nullptr;
}

void SimpleParserObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    data.saveToXML(xml);
}

TaskObject::PreparationError SimpleParserObject::getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const
{
    QString err;
    if (!data.validate(err)) {
        // validation failed
        return PreparationError(err);
    }

    // everything good
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)

    TaskInput textIn;
    textIn.inputName.clear();
    textIn.flags = InputFlag::NoInputFlag;
    textIn.acceptedType.push_back(ObjectType::Data_PlainText);
    in.push_back(textIn);

    TaskOutput treeOut;
    treeOut.outputName.clear();
    treeOut.flags = OutputFlag::NoOutputFlag;
    treeOut.ty = ObjectType::Data_GeneralTree;
    out.push_back(treeOut);

    return PreparationError();
}

SimpleParserExecuteObject* SimpleParserObject::getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const
{
    Q_UNUSED(options)
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)
    return new class SimpleParserExecuteObject(data, getName());
}

//-----------------------------------------------------------------------------

SimpleParserExecuteObject::SimpleParserExecuteObject(const SimpleParser::Data& dataArg, QString name)
    : ExecuteObject(ObjectType::Exec_SimpleParser, name), parser(dataArg)
{

}

void SimpleParserExecuteObject::setInput(QString inputName, ObjectBase* obj)
{
    Q_ASSERT(obj->getType() == ObjectType::Data_PlainText);
    Q_ASSERT(inputName.isEmpty());
    PlainTextObject* textObj = qobject_cast<PlainTextObject*>(obj);
    Q_ASSERT(textObj);
    text = textObj->getText();
}

int SimpleParserExecuteObject::startImpl(ExitCause& cause)
{
    if (isTerminationRequested(cause)) {
        return -1;
    }

    treeOut = Tree();
    bool isGood = parser.performParsing(text, treeOut, logger);
    if (Q_UNLIKELY(!isGood)) {
        return 1;
    }
    GeneralTreeObject* output = new GeneralTreeObject(treeOut);
    emit outputAvailable(QString(), output);
    return 0;
}
