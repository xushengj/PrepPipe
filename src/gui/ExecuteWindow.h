#ifndef EXECUTEWINDOW_H
#define EXECUTEWINDOW_H

#include "src/lib/TaskObject.h"
#include "src/lib/ObjectContext.h"
#include "src/lib/Tree/Configuration.h"

#include <QMainWindow>
#include <QProgressBar>
#include <QCheckBox>
#include <QTreeWidgetItem>
#include <QIcon>
#include <QThread>

namespace Ui {
class ExecuteWindow;
}

class EditorWindow;
class ExecuteWindow : public QMainWindow
{
    Q_OBJECT

public:

    virtual ~ExecuteWindow() override;

    static ExecuteWindow* tryExecuteTask(const ObjectBase::NamedReference& task,
            const TaskObject::LaunchOptions& options,
            const ConfigurationData &taskConfig,
            const QHash<QString, QString>& input,
            EditorWindow* parent = nullptr);

    void requestSwitchToExecuteObject(QTreeWidgetItem* item, ExecuteObject* obj);

private:
    explicit ExecuteWindow(
            ExecuteObject* top,
            const ObjectBase::NamedReference& taskRef,
            const TaskObject::LaunchOptions& options,
            const ConfigurationData& config,
            const QList<TaskObject::TaskInput>& taskInput,
            const QList<TaskObject::TaskOutput>& taskOutput,
            const QHash<QString, QString>& input,
            EditorWindow* parent = nullptr);
    void addExecuteObjects(ExecuteObject* top, QTreeWidgetItem* item);

private slots:
    void initialize();
    void executionThreadFatalEventSlot();
    void finalize(int rootExitCode, ExecuteObject::ExitCause cause);
    void writeBackOutputs();
    void updateCurrentExecutionStatus(const QString& description, int start, int end, int value);
    
protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    Ui::ExecuteWindow *ui;
    EditorWindow* editor;
    QProgressBar* progressBar;
    QCheckBox* followExecObjectCheckBox;
    QTreeWidgetItem* executeObjectRoot;
    QTreeWidgetItem* inputDataRoot;
    QTreeWidgetItem* outputDataRoot;
    QIcon pendingExIcon;
    QIcon currentExIcon;
    QIcon completedExIcon;
    QIcon failedExIcon;
    ExecuteObject* executeRoot;
    QHash<QTreeWidgetItem*, ExecuteObject*> executeObjects; // it also include the root execute object
    ObjectContext outputObjectContext;
    TaskObject::LaunchOptions launchOptions;
    ConfigurationData config;
    const ObjectBase::NamedReference task;
    const QList<TaskObject::TaskInput> taskInputDecl;
    const QList<TaskObject::TaskOutput> taskOutputDecl;
    QHash<QString, QString> specifiedInput;
    QThread* executeThread = nullptr;
    QList<ObjectBase*> pendingOutput;
    bool isRunning = false;
    bool isExitRequested = false;
    bool isFatalEventOccurred = false;
    bool isAnyExecutionFailed = false;
};

#endif // EXECUTEWINDOW_H
