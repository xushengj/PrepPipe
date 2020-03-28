#include "SimpleTextGeneratorObject.h"

#include "src/lib/DataObject/GeneralTreeObject.h"
#include "src/lib/DataObject/PlainTextObject.h"

SimpleTextGeneratorObject::SimpleTextGeneratorObject(const SimpleTextGenerator::Data &dataArg)
    : TaskObject(ObjectType::Task_SimpleTextGenerator), data(dataArg)
{

}

SimpleTextGeneratorObject::SimpleTextGeneratorObject()
    : TaskObject(ObjectType::Task_SimpleTextGenerator)
{

}

SimpleTextGeneratorObject* SimpleTextGeneratorObject::loadFromXML(QXmlStreamReader& xml, StringCache &strCache)
{
    SimpleTextGenerator::Data data;
    if (Q_LIKELY(data.loadFromXML(xml, strCache))) {
        return new SimpleTextGeneratorObject(data);
    }
    return nullptr;
}

void SimpleTextGeneratorObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    data.saveToXML(xml);
}

TaskObject::PreparationError SimpleTextGeneratorObject::getInputOutputInfo(
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

    TaskInput treeIn;
    treeIn.inputName.clear();
    treeIn.flags = InputFlag::NoInputFlag;
    treeIn.acceptedType.push_back(ObjectType::Data_GeneralTree);
    in.push_back(treeIn);

    TaskOutput textOut;
    textOut.outputName.clear();
    textOut.flags = OutputFlag::NoOutputFlag;
    textOut.ty = ObjectType::Data_PlainText;
    out.push_back(textOut);

    return PreparationError();
}

SimpleTextGeneratorExecuteObject* SimpleTextGeneratorObject::getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const
{
    Q_UNUSED(options)
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)
    return new class SimpleTextGeneratorExecuteObject(data, getName());
}

//-----------------------------------------------------------------------------

SimpleTextGeneratorExecuteObject::SimpleTextGeneratorExecuteObject(const SimpleTextGenerator::Data& dataArg, QString name)
    : ExecuteObject(ObjectType::Exec_SimpleTextGenerator, name), generator(dataArg)
{

}

void SimpleTextGeneratorExecuteObject::setInput(QString inputName, ObjectBase* obj)
{
    Q_ASSERT(obj->getType() == ObjectType::Data_GeneralTree);
    Q_ASSERT(inputName.isEmpty());
    GeneralTreeObject* treeObj = qobject_cast<GeneralTreeObject*>(obj);
    Q_ASSERT(treeObj);
    src = treeObj->getTreeData();
}

int SimpleTextGeneratorExecuteObject::startImpl(ExitCause& cause)
{
    if (isTerminationRequested(cause)) {
        return -1;
    }

    QString textOut;
    bool isGood = generator.performGeneration(src, textOut);
    if (Q_UNLIKELY(!isGood)) {
        return 1;
    }
    PlainTextObject* output = new PlainTextObject(textOut);
    emit outputAvailable(QString(), output);
    return 0;
}

