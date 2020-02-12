#include "SimpleTreeTransformObject.h"
#include "src/lib/DataObject/GeneralTreeObject.h"

SimpleTreeTransformObject::SimpleTreeTransformObject(const ConstructOptions &opt)
    : TaskObject(ObjectType::Task_SimpleTreeTransform, opt)
{

}

SimpleTreeTransformObject::SimpleTreeTransformObject(const SimpleTreeTransform::Data& dataArg, const ConstructOptions& opt)
    : TaskObject(ObjectType::Task_SimpleTreeTransform, opt), data(dataArg)
{

}

void SimpleTreeTransformObject::saveToXMLImpl(QXmlStreamWriter &xml)
{
    data.saveToXML(xml);
}

SimpleTreeTransformObject* SimpleTreeTransformObject::loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt)
{
    SimpleTreeTransform::Data data;
    StringCache strCache;
    if (Q_LIKELY(data.loadFromXML(xml, strCache))) {
        return new SimpleTreeTransformObject(data, opt);
    }
    return nullptr;
}

TaskObject::PreparationError SimpleTreeTransformObject::getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const
{
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)

    TaskInput treeIn;
    treeIn.inputName.clear();
    treeIn.flags = InputFlag::NoInputFlag;
    treeIn.acceptedType.push_back(ObjectType::Data_GeneralTree);
    in.push_back(treeIn);
    for (const auto& sideTree : data.sideTreeNameList) {
        TaskInput sideTreeIn;
        sideTreeIn.inputName = sideTree;
        sideTreeIn.flags = InputFlag::NoInputFlag;
        sideTreeIn.acceptedType.push_back(ObjectType::Data_GeneralTree);
    }

    TaskOutput treeOut;
    treeOut.outputName.clear();
    treeOut.flags = OutputFlag::NoOutputFlag;
    treeOut.ty = ObjectType::Data_GeneralTree;
    out.push_back(treeOut);

    return PreparationError();
}

SimpleTreeTransformExecuteObject* SimpleTreeTransformObject::getExecuteObject(
    const LaunchOptions& options,
    const ConfigurationData& config,
    std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
    ) const
{
    Q_UNUSED(options)
    Q_UNUSED(config)
    Q_UNUSED(resolveReferenceCB)
    return new class SimpleTreeTransformExecuteObject(data, getName());
}

//-----------------------------------------------------------------------------

SimpleTreeTransformExecuteObject::SimpleTreeTransformExecuteObject(const SimpleTreeTransform::Data& dataArg, QString name)
    : ExecuteObject(ObjectBase::Exec_SimpleTreeTransform, ConstructOptions(name, QString())), data(dataArg)
{
    for (int i = 0, n = data.sideTreeNameList.size(); i < n; ++i) {
        sideTreeList.push_back(Tree());
    }
}

SimpleTreeTransformExecuteObject::~SimpleTreeTransformExecuteObject()
{

}

void SimpleTreeTransformExecuteObject::setInput(QString inputName, ObjectBase* obj)
{
    Q_ASSERT(inputName.isEmpty());
    Q_ASSERT(obj->getType() == ObjectType::Data_GeneralTree);
    GeneralTreeObject* tree = qobject_cast<GeneralTreeObject*>(obj);
    if (inputName.isEmpty()) {
        treeIn = tree->getTreeData();
    } else {
        int index = data.sideTreeNameList.indexOf(inputName);
        Q_ASSERT(index >= 0 && index < sideTreeList.size());
        sideTreeList[index] = tree->getTreeData();
    }
}

int SimpleTreeTransformExecuteObject::startImpl(ExitCause& cause)
{
    if (isTerminationRequested(cause)) {
        return -1;
    }

    Tree treeOut;
    SimpleTreeTransform transform(data);
    QList<const Tree*> sideTreePtrList;
    for (int i = 0, n = sideTreeList.size(); i < n; ++i) {
        sideTreePtrList.push_back(&sideTreeList.at(i));
    }
    transform.performTransform(treeIn, treeOut, sideTreePtrList);
    return 0;
}
