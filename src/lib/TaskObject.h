#ifndef TASKOBJECT_H
#define TASKOBJECT_H

#include "src/lib/IntrinsicObject.h"
#include "src/lib/ExecuteObject.h"
#include "src/lib/Tree/Configuration.h"
#include "src/lib/ObjectContext.h"

#include <QObject>

class TaskObject : public IntrinsicObject
{
    Q_OBJECT
public:
    TaskObject(ObjectType ty, const ConstructOptions& opt);

    enum LaunchFlag {
        NoLaunchFlag                    = 0x0000,
        Verbose                         = 0x0001, // ExecuteObjects will record more info for UI display (basically whether in debug mode)
        InitialCheck_SilentError        = 0x0002,
        InputAssign_NoStartDialog       = 0x0004, // will fail to execute if not all inputs are specified
        Run_PauseOnStepStart            = 0x0010, // whether there is a breakpoint on start of each execution
        Run_DeleteObjectAfterLastUse    = 0x0020,
        Finalize_AutoTransferObject     = 0x1000, // if editor parent is present, transfer an output object to editor after its last use
        Finalize_AutoCloseIfSuccess     = 0x2000,
        Finalize_AutoCloseIfFail        = 0x4000
    };
    Q_DECLARE_FLAGS(LaunchFlags, LaunchFlag)

    struct LaunchOptions {
        // options that are shared across all executions or with ExecuteWindow
        LaunchFlags flags;
    };

    struct PreparationError {
        enum class Cause {
            NoError,
            InvalidTask,
            UnresolvedReference
        };
        Cause cause = Cause::NoError;
        QString firstFatalErrorDescription;
        ObjectBase::NamedReference firstUnresolvedReference;
    };

    enum InputFlag {
        NoInputFlag             = 0x0,
        ReservedInputFlag       = 0x1, // not used
        AcceptBatch             = 0x2,
        Batch_AcceptEmpty       = 0x4
    };
    Q_DECLARE_FLAGS(InputFlags, InputFlag)

    // only anonymous input should be specified
    struct TaskInput {
        QString inputName;
        InputFlags flags;
        QVector<ObjectType> acceptedType;
    };

    enum OutputFlag {
        NoOutputFlag = 0x0,
        TemporaryOutput = 0x1, // output can be safely discarded
        ProduceBatch = 0x2,
        ObjectTypeSameAsInput = 0x4 // which means that output object type can change for different input types
    };
    Q_DECLARE_FLAGS(OutputFlags, OutputFlag)

    struct TaskOutput {
        QString outputName;
        OutputFlag flags;
        ObjectType ty; // not used if ObjectTypeSameAsInput is set
    };

    struct TaskDependence {
        ObjectBase::NamedReference task;
        QHash<QString, QString> presets;
    };

    virtual const ConfigurationDeclaration* getConfigurationDeclaration() const {return nullptr;} // nullptr for no config

    virtual const ConfigurationData& getDefaultConfig() const {
        qFatal("No default config for objects without config");
        Q_UNREACHABLE();
    }


    virtual PreparationError getInputOutputInfo(
            const ConfigurationData& config,
            QList<TaskInput>& in,
            QList<TaskOutput>& out,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB) const = 0;

    // only called after validation
    // must copy all data needed from named object references; they aren't available afterwards
    // (only run time inputs are available)
    virtual ExecuteObject* getExecuteObject(
            const LaunchOptions& options,
            const ConfigurationData& config,
            std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB
            ) const = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TaskObject::LaunchFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(TaskObject::InputFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(TaskObject::OutputFlags)

#endif // TASKOBJECT_H
