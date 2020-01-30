#ifndef TASKOBJECT_H
#define TASKOBJECT_H

#include "src/lib/IntrinsicObject.h"
#include "src/lib/ExecuteObject.h"
#include "src/lib/Tree/TreeConstraint.h"
#include "src/lib/ObjectContext.h"

#include <QObject>

class TaskObject : public IntrinsicObject
{
    Q_OBJECT
public:
    TaskObject(ObjectType ty, const ConstructOptions& opt);

    struct ObjectNamedReference {
        QStringList nameSpace;
        QString name;
    };

    struct PreparationError {
        enum class Cause {
            NoError,
            InvalidTask,
            UnresolvedReference
        };
        Cause cause;
        ObjectNamedReference firstUnresolvedReference;
        QString firstFatalErrorDescription;
    };

    enum InputRequirementFlag {
        NoInputFlag = 0x0,
        NamedReference = 0x1,
        AcceptBatch = 0x2,
        Batch_AcceptEmpty = 0x4,
        Batch_AcceptMixedType = 0x8
    };
    Q_DECLARE_FLAGS(InputRequirementFlags, InputRequirementFlag)

    struct TaskInputRequirement {
        QString inputName;
        InputRequirementFlags flags;
        QVector<ObjectType> acceptedType;
        TreeConstraint generalTreeConstraint; // only used if only expecting GeneralTreeObject inputs
    };

    virtual bool tryGetInputRequirements(QList<TaskInputRequirement>& result, const ObjectContext& ctx, PreparationError& err) const = 0;

    enum OutputDeclarationFlag {
        NoOutputFlag = 0x0,
        ProduceBatch = 0x1,
        ObjectTypeFixed = 0x02,
        ObjectTypeSameAsInput = 0x04
    };
    Q_DECLARE_FLAGS(OutputDeclarationFlags, OutputDeclarationFlag)

    struct TaskOutputDeclaration {
        QString outputName;
        OutputDeclarationFlag flags;
        ObjectType ty; // not used if ObjectTypeSameAsInput is set
    };

    virtual bool tryGetOutputDeclaration(QList<TaskOutputDeclaration>& result, const ObjectContext& ctx, PreparationError& err) const = 0;

    // only called after validation
    virtual ExecuteObject* getExecuteObject() const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TaskObject::InputRequirementFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(TaskObject::OutputDeclarationFlags)

#endif // TASKOBJECT_H
