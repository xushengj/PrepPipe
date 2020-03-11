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

    void requestSwitchToExecuteObject(QTreeWidgetItem* item, ExecuteObject* obj);

    enum class OutputDestinationType{
        Editor,
        File
    };
    struct OutputAction{
        OutputDestinationType dest;
        QString fileDest_path; // only used for files
    };

public:
    static ExecuteWindow* tryExecuteTask(const ObjectBase::NamedReference& task,
            TaskObject::LaunchOptions &options,
            ConfigurationData &taskConfig,
            QHash<QString, QList<ObjectContext::AnonymousObjectReference> > &input,
            QHash<QString, QVector<OutputAction> > &output,
            EditorWindow* parent = nullptr);

private:
    explicit ExecuteWindow(
            ExecuteObject* top,
            const ObjectBase::NamedReference& taskRef,
            const TaskObject::LaunchOptions& options,
            const QHash<QString, QList<ObjectBase*>>& taskInput,
            const QHash<QString, QVector<OutputAction>>& taskOutput,
            EditorWindow* parent = nullptr);
    void addExecuteObjects(ExecuteObject* top, QTreeWidgetItem* item);

    // for drag/drop
    bool getObjectReference(QTreeWidgetItem* item, ObjectContext::AnonymousObjectReference& ref);

private slots:
    void initialize();
    void executionThreadFatalEventSlot();
    void finalize(int rootExitCode, int cause);
    void writeBackOutputs();
    void updateCurrentExecutionStatus(const QString& description, int start, int end, int value);
    void handleOutput(const QString &outputName, ObjectBase* obj);
    
protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    enum class OriginContext : int {
        Execute         = 0, // this is not backed by an object context
        InputContext    = 1,
        OutputContext   = 2
    };
    struct ObjectListItemData {
        ObjectBase* obj = nullptr;
        QTreeWidgetItem* item = nullptr;
        QWidget* widget = nullptr;
        OriginContext origin = OriginContext::Execute;
    };
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
    const ObjectBase::NamedReference task;
    const QHash<QString, QVector<OutputAction>> taskOutputDecl;
    ObjectContext clonedTaskInputs;
    QHash<QTreeWidgetItem*, ObjectListItemData> itemData;
    QThread* executeThread = nullptr;
    QList<ObjectBase*> pendingOutput;
    bool isRunning = false;
    bool isExitRequested = false;
    bool isFatalEventOccurred = false;
    bool isAnyExecutionFailed = false;
};

#endif // EXECUTEWINDOW_H
