#ifndef SIMPLETREETRANSFORMOBJECT_H
#define SIMPLETREETRANSFORMOBJECT_H

#include "src/lib/IntrinsicObject.h"
#include "src/lib/TaskObject.h"
#include "src/lib/Tree/SimpleTreeTransform.h"
#include "src/lib/Tree/Tree.h"

#include <QObject>

class SimpleTreeTransformExecuteObject : public ExecuteObject
{
    Q_OBJECT
public:
    SimpleTreeTransformExecuteObject(const SimpleTreeTransform::Data& dataArg, QString name);
    virtual ~SimpleTreeTransformExecuteObject() override;

    virtual void setInput(QString inputName, ObjectBase* obj) override;

protected:
    virtual int startImpl(ExitCause& cause) override;

private:
    SimpleTreeTransform::Data data;
    Tree treeIn;
    QList<Tree> sideTreeList;
};

class SimpleTreeTransformObject : public TaskObject
{
    Q_OBJECT
public:

    SimpleTreeTransformObject(const ConstructOptions& opt);
    SimpleTreeTransformObject(const SimpleTreeTransform::Data& dataArg, const ConstructOptions& opt);

    static SimpleTreeTransformObject* loadFromXML(QXmlStreamReader& xml, const ConstructOptions& opt);

    virtual PreparationError getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const override;

    virtual SimpleTreeTransformExecuteObject* getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const override;

protected:
    virtual void saveToXMLImpl(QXmlStreamWriter &xml) override;

private:
    SimpleTreeTransform::Data data;
};

#endif // SIMPLETREETRANSFORMOBJECT_H
