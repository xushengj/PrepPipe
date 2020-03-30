#include "ExecuteOptionDialog.h"
#include "ui_ExecuteOptionDialog.h"

#include <QLabel>
#include <QMessageBox>

ExecuteOptionDialog::ExecuteOptionDialog(
    const TaskObject* task,
    const QStringList& taskRootNameSpace,
    const ObjectContext* taskCtxPtr, // where named references can be resolved

    const TaskObject::LaunchOptions& prevOptions,
    const ConfigurationData& prevConfig,
    const QHash<QString, QList<ObjectContext::AnonymousObjectReference>>& prevInputs,
    const QHash<QString, QVector<ExecuteWindow::OutputAction>>& prevOutputActions,

    QWidget *parent
) :
    QDialog(parent),
    ui(new Ui::ExecuteOptionDialog),

    taskPtr(task),
    rootNameSpace(taskRootNameSpace),
    ctxPtr(taskCtxPtr),

    options(prevOptions),
    taskConfig(prevConfig),
    inputs(prevInputs),
    outputActions(prevOutputActions),

    resolveReferenceCB(std::bind(ObjectContext::resolveNamedReference, std::placeholders::_1, rootNameSpace, ctxPtr)),

    inputLayout(new QFormLayout)
{
    ui->setupUi(this);
    ui->inputGroupBox->setLayout(inputLayout);
    QObject::connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ExecuteOptionDialog::tryAccept);

    const ConfigurationDeclaration* configDecl = nullptr;
    if (const ConfigurationDeclaration* decl = task->getConfigurationDeclaration()) {
        configDecl = decl;
        if (!taskConfig.isValid(*decl)) {
            // something bad is happening, probably because the type of task object changed
            // just reset the config if this happens
            taskConfig = ConfigurationData();
        }
    }

    refreshTaskRequirements();
    updateInputFields();
    // TODO

    /*
     * Original code from wrapper
     *
     *         if (Q_UNLIKELY(rootErr.cause != TaskObject::PreparationError::Cause::NoError)) {
            switch(rootErr.cause){
            case TaskObject::PreparationError::Cause::NoError: {
                Q_UNREACHABLE();
            }break;
            case TaskObject::PreparationError::Cause::InvalidTask: {
                QMessageBox::critical(editor,
                                      tr("Task not ready"),
                                      rootErr.firstFatalErrorDescription);
            }break;
            case TaskObject::PreparationError::Cause::UnresolvedReference: {
                if (rootErr.firstUnresolvedReference.nameSpace.isEmpty()) {
                    rootErr.firstUnresolvedReference.nameSpace = rootNameSpace;
                }
                QMessageBox::critical(editor,
                                      tr("Object not found"),
                                      tr("Dependent object %1 is not found").arg(
                                          rootErr.firstUnresolvedReference.prettyPrint()
                                      ));
            }break;
            }
            return nullptr;
        }
    */
    // for each previous inputs, erase the ones with invalid references
}

ExecuteOptionDialog::~ExecuteOptionDialog()
{
    delete ui;
}

void ExecuteOptionDialog::refreshTaskRequirements()
{
    decltype (taskIn) newTaskIn;
    decltype (taskOut) newTaskOut;

    taskErr = taskPtr->getInputOutputInfo(taskConfig, newTaskIn, newTaskOut, resolveReferenceCB);

    if (!taskErr) {
        taskIn.swap(newTaskIn);
        taskOut.swap(newTaskOut);
        taskInputNameToIndex.clear();
        taskOutputNameToIndex.clear();
        for (int i = 0, n = taskIn.size(); i < n; ++i) {
            taskInputNameToIndex.insert(taskIn.at(i).inputName, i);
        }
        for (int i = 0, n = taskOut.size(); i < n; ++i) {
            taskOutputNameToIndex.insert(taskOut.at(i).outputName, i);
        }
    }
}

ExecuteObject* ExecuteOptionDialog::getExecuteObject() const
{
    return taskPtr->getExecuteObject(options, taskConfig, resolveReferenceCB);
}

bool ExecuteOptionDialog::tryResolveAllReferences(QString& failedInputName)
{
    failedInputName.clear();
    inputs.clear();
    inputObjects.clear();

    // start with scalar inputs
    for (auto iter = inputEdits.begin(), iterEnd = inputEdits.end(); iter != iterEnd; ++iter) {
        const auto& taskInput = taskIn.at(taskInputNameToIndex.value(iter.key(), -1));
        ObjectInputEdit* edit = iter.value();
        ObjectContext::AnonymousObjectReference ref = edit->getResult();
        bool isGood = false;
        if (ref.isValid()) {
            ObjectBase* ptr = ObjectContext::resolveAnonymousReference(ref);
            if (Q_LIKELY(ptr && taskInput.acceptedType.contains(ptr->getType()))) {
                isGood = true;
                QList<ObjectContext::AnonymousObjectReference> refList;
                refList.push_back(ref);
                inputs.insert(iter.key(), refList);
                QList<ObjectBase*> ptrList;
                ptrList.push_back(ptr);
                inputObjects.insert(iter.key(), ptrList);
            }
        } else if (taskInput.flags & TaskObject::InputFlag::InputIsOptional){
            continue;
        }
        if (!isGood) {
            inputs.clear();
            inputObjects.clear();
            failedInputName = iter.key();
            return false;
        }
    }

    // then batches
    // TODO batch not implemented yet
    return true;
}

void ExecuteOptionDialog::updateInputFields()
{
    // step 1: create a new QFormLayout and populate it
    QFormLayout* newLayout = new QFormLayout;
    decltype(inputEdits) newInputEdits;
    for (const auto& taskInput : taskIn) {
        QString inputName = TaskObject::getInputDisplayName(taskInput.inputName);
        QLabel* label = new QLabel(inputName);
        QWidget* field = nullptr;
        if (taskInput.flags & TaskObject::InputFlag::AcceptBatch) {
            // TODO
            Q_ASSERT(0 && "Input field for batch input not implemented yet");
        } else {
            ObjectContext::AnonymousObjectReference ref;
            {
                auto iterIn = inputs.find(taskInput.inputName);
                if (iterIn != inputs.end() && iterIn.value().size() == 1) {
                    ref = iterIn.value().front();
                }
                auto iter = inputEdits.find(taskInput.inputName);
                if (iter != inputEdits.end()) {
                    ref = iter.value()->getResult();
                }
            }
            ObjectInputEdit* edit = new ObjectInputEdit;
            edit->setInputRequirement(taskInput.acceptedType);
            edit->trySetReference(ref);
            newInputEdits.insert(taskInput.inputName, edit);
            field = edit;
        }
        newLayout->addRow(label, field);
    }

    // step 2: tear down old layout
    QLayout* oldLayout = ui->inputGroupBox->layout();
    while (QLayoutItem* item = oldLayout->takeAt(0)) {
        if (QWidget* widget = item->widget())
            widget->deleteLater();
        delete item;
    }
    delete oldLayout;

    // finish up
    ui->inputGroupBox->setLayout(newLayout);
    inputEdits.swap(newInputEdits);
}

void ExecuteOptionDialog::tryAccept()
{
    QString failedParamName;
    if (!tryResolveAllReferences(failedParamName)) {
        QMessageBox::warning(this, tr("Invalid input"), tr("Input for %1 is invalid").arg(TaskObject::getInputDisplayName(failedParamName)));
        return;
    }

    // everything good
    accept();
}
