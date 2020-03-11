#ifndef EXECUTEOPTIONDIALOG_H
#define EXECUTEOPTIONDIALOG_H

#include <QDialog>
#include <QFormLayout>

#include "src/lib/ObjectContext.h"
#include "src/lib/TaskObject.h"
#include "src/lib/ExecuteObject.h"
#include "src/gui/ExecuteWindow.h"
#include "src/gui/ObjectInputEdit.h"

namespace Ui {
class ExecuteOptionDialog;
}

class ExecuteOptionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExecuteOptionDialog(
            const TaskObject* task,
            const QStringList& taskRootNameSpace,
            const ObjectContext* taskCtxPtr, // where named references can be resolved

            const TaskObject::LaunchOptions& prevOptions,
            const ConfigurationData& prevConfig,
            const QHash<QString, QList<ObjectContext::AnonymousObjectReference>>& prevInputs,
            const QHash<QString, QVector<ExecuteWindow::OutputAction>>& prevOutputActions,

            QWidget *parent);
    ~ExecuteOptionDialog();

    // all of the following functions should be called only after the dialog is accepted
    ExecuteObject* getExecuteObject() const;
    const QHash<QString, QList<ObjectBase*>>& getInputObjects() const {return inputObjects;}

    const TaskObject::LaunchOptions& getLaunchOptions() const {return options;}
    const ConfigurationData& getConfigurationData() const {return taskConfig;}
    const QHash<QString, QList<ObjectContext::AnonymousObjectReference>>& getInputReferences() const {return inputs;}
    const QHash<QString, QVector<ExecuteWindow::OutputAction>>& getOutputActions() const {return outputActions;}

private slots:
    void tryAccept();

private:
    void refreshTaskRequirements();
    void updateInputFields();
    bool tryResolveAllReferences(QString& failedInputName);

private:
    Ui::ExecuteOptionDialog *ui;

    // dialog input data
    const TaskObject* taskPtr;
    const QStringList& rootNameSpace;
    const ObjectContext* ctxPtr;

    // configurations that would be saved outside
    TaskObject::LaunchOptions options;
    ConfigurationData taskConfig;
    QHash<QString, QList<ObjectContext::AnonymousObjectReference>> inputs;
    QHash<QString, QVector<ExecuteWindow::OutputAction>> outputActions;

    // helper
    std::function<ObjectBase*(const ObjectBase::NamedReference&)> resolveReferenceCB;

    // only populated when the dialog finishes
    QHash<QString, QList<ObjectBase*>> inputObjects;

    // state
    QList<TaskObject::TaskInput> taskIn;
    QList<TaskObject::TaskOutput> taskOut;
    TaskObject::PreparationError taskErr;
    QHash<QString, int> taskInputNameToIndex;
    QHash<QString, int> taskOutputNameToIndex;
    QFormLayout* inputLayout;
    QHash<QString, ObjectInputEdit*> inputEdits; // only for "scalar" (not batch) inputs

};

#endif // EXECUTEOPTIONDIALOG_H
