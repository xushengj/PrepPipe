#include "ExecuteWindow.h"
#include "ui_ExecuteWindow.h"

#include "src/gui/EditorWindow.h"
#include "src/gui/ExecuteOptionDialog.h"
#include "src/misc/MessageLogger.h"

#include <QDebug>
#include <QCommonStyle>
#include <QMessageBox>
#include <QCloseEvent>
#include <QEventLoop>
#include <QDateTime>

#include <functional>

ExecuteWindow::ExecuteWindow(ExecuteObject *top, const ObjectBase::NamedReference& taskRef,
                             const TaskObject::LaunchOptions& options,
                             const QHash<QString, QList<ObjectBase *> > &taskInput,
                             const QHash<QString, QVector<OutputAction> > &taskOutput,
                             EditorWindow *parent) :
    QMainWindow(nullptr), // we always want to show execute window as an independent one (without forced to be on top of EditorWindow)
    ui(new Ui::ExecuteWindow),
    editor(parent),
    progressBar(new QProgressBar),
    followExecObjectCheckBox(new QCheckBox(tr("Follow execution"))),
    executeObjectRoot(new QTreeWidgetItem),
    inputDataRoot(new QTreeWidgetItem),
    outputDataRoot(new QTreeWidgetItem),
    pendingExIcon(":/icon/execute/pending.png"),
    currentExIcon(":/icon/execute/current.png"),
    completedExIcon(":/icon/execute/completed.png"),
    failedExIcon(":/icon/execute/failed.png"),
    executeRoot(top),
    outputObjectContext(),
    launchOptions(options),
    task(taskRef),
    taskOutputDecl(taskOutput)
{
    ui->setupUi(this);

    followExecObjectCheckBox->setChecked(true);
    progressBar->setRange(0, 1);
    ui->statusbar->addPermanentWidget(followExecObjectCheckBox, 0);
    ui->statusbar->addPermanentWidget(progressBar, 1);
    ui->statusbar->showMessage(tr("Uninitialized"));

    QCommonStyle style;
    executeObjectRoot->setText(0, top->getName()); // will be updated
    executeObjectRoot->setIcon(0, pendingExIcon);
    inputDataRoot->setText(0, tr("Input data"));
    inputDataRoot->setIcon(0, style.standardIcon(QStyle::SP_DirOpenIcon));
    outputDataRoot->setText(0, tr("Output data"));
    outputDataRoot->setIcon(0, style.standardIcon(QStyle::SP_FileDialogNewFolder));
    ui->objectTreeWidget->insertTopLevelItem(0, executeObjectRoot);
    ui->objectTreeWidget->insertTopLevelItem(1, inputDataRoot);
    ui->objectTreeWidget->insertTopLevelItem(2, outputDataRoot);

    executeThread = MessageLogger::inst()->createThread(tr("Execution thread"), [this]() -> void {
        // warning: this lambda will be executed in executeThread
        // when a fatal event happens, executeThread will just keep spinning in this loop
        // and wait for main thread to terminate it
        QThread::currentThread()->setPriority(QThread::LowestPriority);
        QMetaObject::invokeMethod(this, &ExecuteWindow::executionThreadFatalEventSlot, Qt::QueuedConnection);
        while(true){}
    });

    // populate all dependent executable objects
    addExecuteObjects(top, executeObjectRoot);

    // helper for next stage
    auto getClonedObject = [](ObjectBase* src) -> ObjectBase* {
        ObjectBase* cloned = src->clone();
        cloned->lock();

        // move current name to comments
        QString header = tr("[%1] Origin: \"").arg(QDateTime::currentDateTime().toString());
        header.append(cloned->getName());
        QString path = cloned->getFilePath();
        if (!path.isEmpty()) {
            header.append(" (");
            header.append(path);
            header.append(")");
        }
        header.append('\n');
        header.append(cloned->getComment());
        cloned->setComment(header);
        cloned->setFilePath(QString());

        return cloned;
    };

    // populate all input objects
    for (auto iter = taskInput.begin(), iterEnd = taskInput.end(); iter != iterEnd; ++iter) {
        auto& list = iter.value();
        const QString& key = iter.key();
        QTreeWidgetItem* item = new QTreeWidgetItem(inputDataRoot);
        item->setText(0, key);
        if (list.isEmpty()) {
            // no icon
            top->setInput(key, nullptr);
        } else if (list.size() == 1) {
            ObjectBase* srcObject = list.front();
            ObjectBase* cloned = getClonedObject(srcObject);
            cloned->setName(key);
            item->setIcon(0, cloned->getTypeDisplayIcon());
            ObjectListItemData curItemData;
            curItemData.obj = cloned;
            curItemData.item = item;
            itemData.insert(item, curItemData);
            top->setInput(key, cloned);
            clonedTaskInputs.addObject(cloned);
        } else {
            item->setIcon(0, style.standardIcon(QStyle::SP_DirOpenIcon));
            QList<ObjectBase*> clonedInputs;
            for (int i = 0, n = list.size(); i < n; ++i) {
                ObjectBase* srcObject = list.at(i);
                ObjectBase* cloned = getClonedObject(srcObject);
                QString objName = key;
                objName.append('_');
                objName.append(QString::number(i));
                cloned->setName(objName);
                QTreeWidgetItem* curItem = new QTreeWidgetItem(item);
                curItem->setText(0, objName);
                curItem->setIcon(0, cloned->getTypeDisplayIcon());
                ObjectListItemData curItemData;
                curItemData.obj = cloned;
                curItemData.item = curItem;
                itemData.insert(curItem, curItemData);
                clonedInputs.push_back(cloned);
                clonedTaskInputs.addObject(cloned);
            }
            top->setInput(key, clonedInputs);
        }
    }

    QMetaObject::invokeMethod(this, &ExecuteWindow::initialize, Qt::QueuedConnection);
}

void ExecuteWindow::addExecuteObjects(ExecuteObject* top, QTreeWidgetItem* item)
{
    top->moveToThread(executeThread);
    ObjectListItemData curItemData;
    curItemData.obj = top;
    curItemData.item = item;
    itemData.insert(item, curItemData);
    
    auto startHandlerLambda = [this, top, item]() -> void{
        qInfo() << "Execution of" << top->getName() << "started";
        item->setIcon(0, currentExIcon);
        if (followExecObjectCheckBox->isChecked()) {
            requestSwitchToExecuteObject(item, top);
        }
    };
    Q_ASSERT(connect(top, &ExecuteObject::started, this, startHandlerLambda, Qt::QueuedConnection));

    auto finishHandlerLambda = [this, top, item](int status, int causeInt)->void {
        ExecuteObject::ExitCause cause = static_cast<ExecuteObject::ExitCause>(causeInt);
        qInfo() << "Execution of" << top->getName()
                << "finished with exit code " << status
                << " and cause " << causeInt;
        if (cause == ExecuteObject::ExitCause::Completed && status == 0) {
            item->setIcon(0, completedExIcon);
        } else {
            item->setIcon(0, failedExIcon);
            if (!isAnyExecutionFailed) {
                isAnyExecutionFailed = true;
            } else {
                // switch only if this is the first failed object
                // (failure of child will probably result in failure of parent soon)
                if (followExecObjectCheckBox->isChecked()) {
                    requestSwitchToExecuteObject(item, top);
                }
            }
        }
        if (top == executeRoot) {
            followExecObjectCheckBox->setCheckable(false);
        }
    };
    Q_ASSERT(connect(top, &ExecuteObject::finished, this, finishHandlerLambda, Qt::QueuedConnection));

    executeObjects.insert(item, top);

    QList<ExecuteObject*> children;
    top->getChildExecuteObjects(children);
    int numChild = children.size();
    qInfo() << "ExecuteObject" << top->getName() << "added with " << numChild << "dependence";
    for (int i = 0; i < numChild; ++i) {
        ExecuteObject* child = children.at(i);
        QTreeWidgetItem* childItem = new QTreeWidgetItem(item);
        childItem->setText(0, child->getName());
        childItem->setIcon(0, pendingExIcon);
        addExecuteObjects(child, childItem);
    }
}

void ExecuteWindow::requestSwitchToExecuteObject(QTreeWidgetItem* item, ExecuteObject* obj)
{
    // TODO
    Q_UNUSED(item)
    Q_UNUSED(obj)
}

ExecuteWindow* ExecuteWindow::tryExecuteTask(const ObjectBase::NamedReference &taskRef,
        TaskObject::LaunchOptions& options,
        ConfigurationData& taskConfig,
        QHash<QString, QList<ObjectContext::AnonymousObjectReference>>& input,
        QHash<QString, QVector<ExecuteWindow::OutputAction>>& output,
        EditorWindow* editor)
{
    const ObjectContext* ctxPtr = editor? &editor->getMainContext() : nullptr;
    ObjectBase* taskObj = ObjectContext::resolveNamedReference(taskRef, QStringList(), ctxPtr);
    if (Q_UNLIKELY(!taskObj)) {
        qCritical().noquote().nospace() << "Object " << taskRef.prettyPrint() << " is not found";
        if (!(options.flags & TaskObject::InitialCheck_SilentError)) {
            QMessageBox::critical(editor,
                                  tr("Object not found"),
                                  tr("Task %1 is not found and execution cannot continue.").arg(
                                      taskRef.prettyPrint())
                                  );
        }
        return nullptr;
    }

    TaskObject* task = qobject_cast<TaskObject*>(taskObj);
    if (Q_UNLIKELY(!task)) {
        qCritical().noquote().nospace() << "Object " << taskRef.prettyPrint() << " is not a task object";
        if (!(options.flags & TaskObject::InitialCheck_SilentError)) {
            QMessageBox::critical(editor,
                                  tr("Invalid entry task"),
                                  tr("Object %1 cannot be runned because it is not a task.").arg(
                                      taskRef.prettyPrint())
                                  );
        }
        return nullptr;
    }

    // okay, task object is valid
    // pop up the dialog to fill remaining things
    ExecuteOptionDialog* dialog = new ExecuteOptionDialog(
                task,
                taskRef.nameSpace,
                ctxPtr,
                options,
                taskConfig,
                QHash<QString, QList<ObjectContext::AnonymousObjectReference>>(),
                QHash<QString, QVector<OutputAction>>(),
                editor);

    // wait until the dialog is done
    QEventLoop localLoop;
    QObject::connect(dialog, &QDialog::finished, &localLoop, &QEventLoop::quit);
    dialog->show();
    localLoop.exec();

    if (dialog->result() == QDialog::Rejected)
        return nullptr;

    // okay, all good
    // get what we had from dialog
    options = dialog->getLaunchOptions();
    taskConfig = dialog->getConfigurationData();
    input = dialog->getInputReferences();
    output = dialog->getOutputActions();

    // we can derive ExecuteObjects now
    ExecuteObject* execObj = dialog->getExecuteObject();
    Q_ASSERT(execObj);

    ExecuteWindow* window = new ExecuteWindow(execObj, taskRef, options, dialog->getInputObjects(), output, editor);
    dialog->deleteLater();
    window->show();
    return window;
}

void ExecuteWindow::initialize()
{
    // this is invoked by a queued connection from constructor
    // launch the task
    connect(executeRoot, &ExecuteObject::statusUpdate, this, &ExecuteWindow::updateCurrentExecutionStatus, Qt::QueuedConnection);
    connect(executeRoot, &ExecuteObject::finished, this, &ExecuteWindow::finalize, Qt::QueuedConnection);
    QMetaObject::invokeMethod(executeRoot, &ExecuteObject::start, Qt::QueuedConnection);
    executeThread->start();
    isRunning = true;
}

void ExecuteWindow::writeBackOutputs()
{
    // TODO
}

void ExecuteWindow::executionThreadFatalEventSlot()
{
    finalize(-1, static_cast<int>(ExecuteObject::ExitCause::FatalEvent));
}

void ExecuteWindow::finalize(int rootExitCode, int causeInt)
{
    ExecuteObject::ExitCause cause = static_cast<ExecuteObject::ExitCause>(causeInt);
    Q_ASSERT(executeThread);
    isRunning = false;
    // stop the thread first
    switch(cause) {
    case ExecuteObject::ExitCause::FatalEvent: {
        executeThread->terminate();
    }break;
    case ExecuteObject::ExitCause::Completed:
    case ExecuteObject::ExitCause::Terminated:{
        executeThread->quit();
    }
    }
    executeThread->wait();

    qInfo() << "Execution finished with exit code " << rootExitCode << " and cause " << static_cast<int>(cause);
    switch(cause) {
    case ExecuteObject::ExitCause::FatalEvent: {
        isFatalEventOccurred = true;
        executeObjectRoot->setIcon(0, failedExIcon);
        disconnect(executeRoot, &ExecuteObject::finished, this, &ExecuteWindow::finalize);
        disconnect(executeRoot, &ExecuteObject::statusUpdate, this, &ExecuteWindow::updateCurrentExecutionStatus);
        QMessageBox* msgBox = new QMessageBox(QMessageBox::Critical,
                                              tr("Fatal error encountered"),
                                              tr("Sorry, a fatal event has occurred and the execution is failed."
                                                 " If it is not due to external factors (e.g. insufficient memory or disk space),"
                                                 " please submit a bug report. The program is in an unstable state, and"
                                                 " please consider saving your work and restarting the program before any further actions."
                                                 " (Note: If the log is large, this program may freeze for a while, and please wait.)"),
                                              QMessageBox::Ok,
                                              this);
        msgBox->show();
        connect(msgBox, &QDialog::finished, msgBox, &QObject::deleteLater);
        // try to make sure the message box is shown before we start to concatenate logs (which can take a while)
        QApplication::processEvents();
        // heavy lifting
        MessageLogger::inst()->cleanupThread(executeThread);
        executeThread = nullptr;
        // done
        MessageLogger::inst()->openLogFile();
    }break;
    case ExecuteObject::ExitCause::Terminated: {
        // do nothing else; just exit
        MessageLogger::inst()->cleanupThread(executeThread);
        executeThread = nullptr;
        close();
        deleteLater();
    }break;
    case ExecuteObject::ExitCause::Completed: {
        MessageLogger::inst()->cleanupThread(executeThread);
        executeThread = nullptr;
        updateCurrentExecutionStatus(tr("Completed"), 0, 1, 1);
        bool isExecutionGood = (rootExitCode == 0);
        bool isAutoClose = isExecutionGood ? (launchOptions.flags & TaskObject::LaunchFlag::Finalize_AutoCloseIfSuccess)
                                           : (launchOptions.flags & TaskObject::LaunchFlag::Finalize_AutoCloseIfFail);
        bool isWriteBack = isAutoClose || (launchOptions.flags & TaskObject::LaunchFlag::Finalize_AutoTransferObject);

        if (isWriteBack && !pendingOutput.isEmpty()) {
            writeBackOutputs();
        }

        if (isAutoClose) {
            close();
        }
    }break;
    }
}

void ExecuteWindow::closeEvent(QCloseEvent* event)
{
    if (!isRunning) {
        if (!pendingOutput.isEmpty()) {
            writeBackOutputs();
        }
        event->accept();
        return;
    }

    // avoid handling multiple user request on termination
    // this must be after the check on isRunning so that finalize() can close the window
    if (isExitRequested) {
        if (QMessageBox::question(
                    this,
                    tr("Confirm force exit"),
                    tr("Do you want to force quit? The entire program will abort.")) == QMessageBox::No) {
            event->ignore();
            return;
        }

        // force quit requested
        // check again first
        if (!isRunning) {
            if (!pendingOutput.isEmpty()) {
                writeBackOutputs();
            }
            event->accept();
            return;
        }
        // okay, force quit..
        abort();
    }

    
    // now we are still running the task; ask whether to cancel
    if (QMessageBox::question(this,
                              tr("Confirm cancel"),
                              tr("The task is still running. Do you want to terminate it?"),
                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::NoButton) == QMessageBox::Cancel){
        event->ignore();
        return;
    }
    
    // now we are requested to cancel
    // no change if the task has already finished before the dialog is answered
    if (!isRunning) {
        event->accept();
        return;
    }
    
    isExitRequested = true;
    disconnect(executeRoot, &ExecuteObject::statusUpdate, this, &ExecuteWindow::updateCurrentExecutionStatus);
    updateCurrentExecutionStatus(tr("Waiting for termination"), 0, 0, 0);
    executeThread->requestInterruption();
    event->ignore();
}

ExecuteWindow::~ExecuteWindow()
{
    delete ui;

    if (executeThread) {
        // we probably just exited the window without launching any task
        Q_ASSERT(!executeThread->isRunning());
        MessageLogger::inst()->cleanupThread(executeThread);
        executeThread = nullptr;
    }

    if (!pendingOutput.isEmpty()) {
        writeBackOutputs();
    }

    // do not clean up execute objects if any fatal event is occurred
    // (data structure can be corrupted)
    if (!isFatalEventOccurred) {
        // delete all ExecuteObjects
        for (auto& p : executeObjects) {
            delete p;
        }
    }
}

void ExecuteWindow::updateCurrentExecutionStatus(const QString& description, int start, int end, int value)
{
    ui->statusbar->showMessage(description);
    progressBar->setRange(start, end);
    progressBar->setValue(value);
}
