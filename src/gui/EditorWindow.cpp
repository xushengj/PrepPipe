#include "EditorWindow.h"
#include "ui_EditorWindow.h"

#include "src/lib/DataObject/MIMEDataObject.h"
#include "src/lib/StaticObjectIndexDB.h"
#include "src/misc/MessageLogger.h"
#include "src/gui/ExecuteWindow.h"
#include "src/gui/ObjectTreeWidget.h"
#include "src/lib/TaskObject.h"
#include "src/gui/DropTestLabel.h"
#include "src/lib/FileBackedObject.h"
#include "src/gui/EditorBase.h"
#include "src/lib/DataObject/PlainTextObject.h"
#include "src/gui/IntrinsicObjectCreationDialog.h"

#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QGridLayout>
#include <QCommonStyle>
#include <QFileDialog>
#include <QDateTime>
#include <QString>
#include <QStringRef>
#include <QInputDialog>
#include <QDesktopServices>

namespace {
const QString SETTINGS_EDITOR_PLACEHOLDER_TEXT = QStringLiteral("editor/placeholder_text");

QString getPlaceHolderText()
{
    QString placeholderText = Settings::inst()->value(SETTINGS_EDITOR_PLACEHOLDER_TEXT).toString();
    if (placeholderText.isEmpty()) {
        placeholderText = EditorWindow::tr("No object is currently opening.");
    }
    return placeholderText;
}
}

EditorWindow::EditorWindow(QString startDirectory, QString startTask, QString startPath, QStringList presets, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::EditorWindow), mainCtx(), sideCtx()
{
    ui->setupUi(this);
    setContextMenuPolicy(Qt::DefaultContextMenu); // call contextMenuEvent()

    ui->placeholderLabel->setText(getPlaceHolderText());
    ui->currentObjectComboBox->addItem(tr("(No object is opened)"));
    ui->currentObjectComboBox->setEnabled(false);

    QCommonStyle style;
    mainRoot = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("In Directory")));
    mainRoot->setIcon(0, style.standardIcon(QStyle::SP_DirHomeIcon));
    mainRoot->setToolTip(0, mainCtx.getDirectory());
    sideRoot = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Other")));
    sideRoot->setIcon(0, style.standardIcon(QStyle::SP_DirIcon));
    ui->objectListTreeWidget->setGetReferenceCallback(std::bind(&EditorWindow::getObjectReference, this, std::placeholders::_1, std::placeholders::_2));
    ui->objectListTreeWidget->insertTopLevelItem(0, mainRoot);
    ui->objectListTreeWidget->insertTopLevelItem(1, sideRoot);

    connect(Settings::inst(), &Settings::settingChanged, this, &EditorWindow::settingChanged);

    ui->objectListTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu); // emit signal
    connect(ui->objectListTreeWidget, &QTreeWidget::itemClicked,                this, &EditorWindow::objectListItemClicked);
    connect(ui->objectListTreeWidget, &QTreeWidget::itemDoubleClicked,          this, &EditorWindow::objectListItemDoubleClicked);
    connect(ui->objectListTreeWidget, &QTreeWidget::customContextMenuRequested, this, &EditorWindow::objectListContextMenuRequested);

    connect(ui->actionChangeDirectory, &QAction::triggered, this, &EditorWindow::changeDirectoryRequested);
    connect(ui->actionReloadDirectory, &QAction::triggered, this, &EditorWindow::reloadFromDirectoryRequested);
    connect(ui->actionCaptureClipboard, &QAction::triggered, this, &EditorWindow::clipboardDumpRequested);
    connect(ui->actionOpenLog, &QAction::triggered, MessageLogger::inst(), &MessageLogger::openLogFile);
    connect(ui->actionOpen, &QAction::triggered, this, &EditorWindow::openFileRequested);
    connect(ui->actionSave, &QAction::triggered, this, &EditorWindow::saveRequested);

    connect(ui->actionQFatal, &QAction::triggered, this, []() -> void {
        qFatal("Fatal event message requested");
    });
    connect(ui->actionSegFault, &QAction::triggered, this, []() -> void {
        *(volatile char*)nullptr;
    });

    connect(ui->actionNewIntrinsicObject,   &QAction::triggered, this, &EditorWindow::createRequest_IntrinsicObject);
    connect(ui->actionNewPlainText,         &QAction::triggered, this, &EditorWindow::createRequest_PlainText);

    connect(ui->dropTestLabel, &DropTestLabel::dataDropped, this, &EditorWindow::dataDropped);
    connect(ui->objectListTreeWidget, &ObjectTreeWidget::objectDropped, this, &EditorWindow::objectDropped);
    ui->objectListTreeWidget->addSelfContextIndex(mainCtx.getContextIndex());
    ui->objectListTreeWidget->addSelfContextIndex(sideCtx.getContextIndex());
    ui->objectListTreeWidget->setAcceptDrops(true);

    setWindowTitle(QApplication::applicationName());

    QMetaObject::invokeMethod(this, &EditorWindow::processDelayedStartupAction, Qt::QueuedConnection);

    // populate startup info
    startup.startDirectory = startDirectory;
    startup.startFile = startPath;
    startup.isStartTaskSpecified = false;

    if (!startTask.isEmpty()) {
        startup.isStartTaskSpecified = true;
        startup.startTask = ObjectBase::NamedReference(startTask);
    }

    for (const auto& str : presets) {
        int pos = str.indexOf('=');
        if (pos == -1) {
            qWarning() << "Unhandled command line option " << str;
            continue;
        }
        QStringRef key = str.midRef(0, pos);
        QStringRef value = str.midRef(pos + 1);
        if (key.length() >= 2 && key.front() == '"' && key.back() == '"') {
            key = key.mid(1).chopped(1);
        }
        if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.mid(1).chopped(1);
        }
        QString keyStr = key.toString();
        QString valueStr = value.toString();
        auto iter = startup.presets.find(keyStr);
        if (iter != startup.presets.end()) {
            qWarning() << "Duplicate value for key " << keyStr <<"; previous value " << iter.value() << "is overwritten";
            iter.value() = valueStr;
        } else {
            startup.presets.insert(keyStr, valueStr);
        }
    }
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::updateObjectListItemForObject(QTreeWidgetItem* item, ObjectBase* obj)
{
    item->setIcon(0, obj->getTypeDisplayIcon());
    item->setText(0, obj->getName());
    QString objTooltip = obj->getTypeDisplayName();
    QString absPath = obj->getFilePath();
    if (!absPath.isEmpty()) {
        objTooltip.append(' ');
        objTooltip.append('(');
        objTooltip.append(absPath);
        objTooltip.append(')');
    }
    item->setToolTip(0, objTooltip);
}

void EditorWindow::populateObjectListTreeFromMainContext()
{
    // we only need to initialize the part for main context
    for (auto objPtr : mainCtx) {
        Q_ASSERT(objPtr && !objPtr->getName().isEmpty());
        QTreeWidgetItem* item = new QTreeWidgetItem(mainRoot);
        updateObjectListItemForObject(item, objPtr);
        itemData.insert(item, ObjectListItemData(objPtr, nullptr, OriginContext::MainContext));
    }
    mainRoot->setExpanded(true);
}

void EditorWindow::settingChanged(const QStringList& keyList)
{
    // update placeholder text if that is one of the changes
    if (keyList.contains(SETTINGS_EDITOR_PLACEHOLDER_TEXT)) {
        ui->placeholderLabel->setText(getPlaceHolderText());
    }
}

bool EditorWindow::getObjectReference(QTreeWidgetItem* item, ObjectContext::AnonymousObjectReference &ref)
{
    auto iter = itemData.find(item);
    if (iter == itemData.end())
        return false;
    auto& data = iter.value();
    switch (data.origin) {
    case OriginContext::MainContext: {
        ref.ctxIndex = mainCtx.getContextIndex();
        ref.refIndex = mainCtx.getObjectReference(data.obj);
    }break;
    case OriginContext::SideContext: {
        ref.ctxIndex = sideCtx.getContextIndex();
        ref.refIndex = sideCtx.getObjectReference(data.obj);
    }break;
    }
    return true;
}

void EditorWindow::objectListItemClicked(QTreeWidgetItem* item, int column)
{
    // do nothing for now
    Q_UNUSED(item)
    Q_UNUSED(column)
}

void EditorWindow::objectListItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column)

    // only trying to request editor open when the item represents an object, instead of a category
    if (itemData.contains(item)) {
        objectListOpenEditorRequested(item);
    }
}

void EditorWindow::objectListOpenEditorRequested(QTreeWidgetItem* item)
{
    auto iter = itemData.find(item);
    Q_ASSERT(iter != itemData.end());
    auto& data = iter.value();
    if (data.editor == nullptr) {
        data.editor = data.obj->getEditor();
    }
    if (data.editor) {
        showObjectEditor(data.obj, data.editor, item, data.origin);
        ui->objectListTreeWidget->setCurrentItem(item);
    } else {
        QMessageBox::information(this, tr("Feature not implemented"), tr("Sorry, editing or viewing this object is not supported."));
    }
}

void EditorWindow::showObjectEditor(ObjectBase* obj, QWidget* editor, QTreeWidgetItem* item, OriginContext origin)
{
    int index = -1;
    // first of all, go through the opened object list to see if there is already one there
    for (int i = 0, n = editorOpenedObjects.size(); i < n; ++i) {
        const auto& data = editorOpenedObjects.at(i);
        if (data.obj == obj) {
            Q_ASSERT(data.editor == editor && data.item == item);
            index = i;
            break;
        }
    }
    if (index == -1) {
        // not opened before; add to list
        index = editorOpenedObjects.size();
        editorOpenedObjects.push_back(EditorOpenedObjectData(obj, editor, item, origin));
    }

    switchToEditor(index);
}

void EditorWindow::switchToEditor(int index)
{
    if (currentOpenedObjectIndex == index)
        return;

    QWidget* previousWidget = (currentOpenedObjectIndex != -1)? editorOpenedObjects.at(currentOpenedObjectIndex).editor : ui->placeholderLabel;
    QWidget* curWidget = (index != -1)? editorOpenedObjects.at(index).editor : ui->placeholderLabel;
    currentOpenedObjectIndex = index;
    ui->placeholderWidget->layout()->removeWidget(previousWidget);
    previousWidget->hide();
    qobject_cast<QGridLayout*>(ui->placeholderWidget->layout())->addWidget(curWidget, 0, 0);
    curWidget->show();

    if (index != -1) {
        if (EditorBase* editor = qobject_cast<EditorBase*>(curWidget)) {
            connect(editor, &EditorBase::dirty, this, &EditorWindow::updateWindowTitle, Qt::UniqueConnection);
        }
    }

    updateWindowTitle();
}

void EditorWindow::updateWindowTitle()
{
    if (currentOpenedObjectIndex == -1) {
        setWindowTitle(QApplication::applicationName());
    } else {
        auto& data = editorOpenedObjects.at(currentOpenedObjectIndex);
        ObjectBase* obj = data.obj;
        QString objTitle;
        if (FileBackedObject* fbo = qobject_cast<FileBackedObject*>(obj)) {
            objTitle = fbo->getFilePath();
        }
        if (objTitle.isEmpty()) {
            objTitle = obj->getName();
        }
        QString title = QString("%1 - %2").arg(objTitle, QApplication::applicationName());

        bool isDirty = false;
        if (EditorBase* editor = qobject_cast<EditorBase*>(data.editor)) {
            isDirty = editor->isDirty();
        }
        if (isDirty) {
            title.prepend('*');
        }
        setWindowTitle(title);
    }
}

void EditorWindow::objectListContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = ui->objectListTreeWidget->itemAt(pos);
    QMenu menu(ui->objectListTreeWidget);

    if (item == mainRoot) {
        menu.addAction(ui->actionChangeDirectory);
        menu.addAction(ui->actionReloadDirectory);
    } else if (item == sideRoot) {
        QAction* closeAllAction = new QAction(tr("Close All"));
        connect(closeAllAction, &QAction::triggered, this, &EditorWindow::tryCloseAllSideContextObjects);
        menu.addAction(closeAllAction);
    } else {
        auto iter = itemData.find(item);
        if (iter != itemData.end()) {
            auto& data = iter.value();
            // menu from specific ones to general ones
            if (TaskObject* task = qobject_cast<TaskObject*>(data.obj)) {
                if (item->parent() == mainRoot) {
                    QAction* executeAction = new QAction(tr("Execute"));
                    connect(executeAction, &QAction::triggered, this, [=]() -> void {
                        saveHelper(task, data.editor);
                        updateWindowTitle();
                        ObjectBase::NamedReference ref;
                        ref.name = task->getName();
                        launchTask(ref);
                    });
                    menu.addAction(executeAction);
                }
            }
            if (FileBackedObject* fbo = qobject_cast<FileBackedObject*>(data.obj)) {
                QAction* saveAction = new QAction(tr("Save"));
                connect(saveAction, &QAction::triggered, this, [=]() -> void {
                    saveHelper(fbo, data.editor);
                    updateWindowTitle();
                });
                menu.addAction(saveAction);

                QString filePath = fbo->getFilePath();
                if (!filePath.isEmpty()) {
                    QFileInfo f(filePath);
                    QString dir = f.dir().absolutePath();
                    QAction* openDirectoryAction = new QAction(tr("Open containing directory"));
                    connect(openDirectoryAction, &QAction::triggered, this, [=]() -> void {
                        QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
                    });
                    menu.addAction(openDirectoryAction);
                }
            }
            if (item->parent() == sideRoot) {
                ObjectBase* obj = data.obj;
                QAction* renameAction = new QAction(tr("Rename"));
                connect(renameAction, &QAction::triggered, this, [=]() -> void {
                    QString name = QInputDialog::getText(this, tr("Rename Object"), tr("Please input the new name:"), QLineEdit::Normal, obj->getName());
                    if (!name.isEmpty() && name != obj->getName()) {
                        obj->setName(name);
                        updateObjectListItemForObject(item, obj);
                        updateWindowTitle();
                    }
                });
                menu.addAction(renameAction);
                QAction* closeAction = new QAction(tr("Close"));
                connect(closeAction, &QAction::triggered, this, [=]() -> void {
                    closeSideContextObjectRequested(item);
                });
                menu.addAction(closeAction);
            }
        } else {
            // right clicking on nowhere
            menu.addAction(ui->actionNewIntrinsicObject);
            menu.addMenu(ui->menuNew_Common_File);
            menu.addMenu(ui->menuCapture);
            menu.addAction(ui->actionOpen);
        }
    }

    if (menu.isEmpty())
        return;

    menu.exec(ui->objectListTreeWidget->mapToGlobal(pos));
}

void EditorWindow::contextMenuEvent(QContextMenuEvent *event)
{
    // TODO
    event->accept();
}

void EditorWindow::changeDirectoryRequested()
{
    QString newDir = QFileDialog::getExistingDirectory(this, tr("Set new working directory"), mainCtx.getDirectory());
    if (newDir.isEmpty() || newDir == mainCtx.getDirectory())
        return;

    changeDirectory(newDir);
}

void EditorWindow::reloadFromDirectoryRequested()
{
    changeDirectory(mainCtx.getDirectory());
}

bool EditorWindow::tryCloseAllObjectsCommon(OriginContext origin)
{
    // if all opened editor belongs to objects that will be removed, switch to placeholder label
    // if current editor do not belong to an object that will be removed, stay with it
    // otherwise, pick any surviving editor

    // WARNING: this function do not delete objects; the context is still untouched

    // loop over all editor items and see which would be preserved and which would be closed
    decltype (editorOpenedObjects) tmpList;
    tmpList.reserve(editorOpenedObjects.size());
    QWidget* editorToSwitch = nullptr;
    if (currentOpenedObjectIndex != -1) {
        if (editorOpenedObjects.at(currentOpenedObjectIndex).origin != origin) {
            // we can keep this one
            editorToSwitch = editorOpenedObjects.at(currentOpenedObjectIndex).editor;
        }
    }
    for (int i = 0, n = editorOpenedObjects.size(); i < n; ++i) {
        const auto& data = editorOpenedObjects.at(i);
        if (data.origin == origin) {
            // we will close this one
            if (!closeEditorCheck(i)) {
                return false;
            }
        } else {
            tmpList.push_back(data);
        }
    }
    switchToEditor(-1);

    // now we are good to close all of them
    tmpList.swap(editorOpenedObjects);

    if (editorToSwitch) {
        for (int i = 0, n = editorOpenedObjects.size(); i < n; ++i) {
            if (editorOpenedObjects.at(i).editor == editorToSwitch) {
                switchToEditor(i);
                break;
            }
        }
    }
    if (currentOpenedObjectIndex == -1 && !editorOpenedObjects.isEmpty()) {
        switchToEditor(0);
    }

    // remove items in object list tree
    for (auto iter = itemData.begin(); iter != itemData.end();) {
        if (iter.value().origin == origin) {
            QTreeWidgetItem* item = iter.key();
            ObjectBase* obj = iter.value().obj;
            QWidget* editor = iter.value().editor;
            if (editor) {
                obj->tearDownEditor(editor);
            }
            delete item;
            iter = itemData.erase(iter);
        } else {
            ++ iter;
        }
    }
    return true;
}

bool EditorWindow::closeEditorCheck(int index)
{
    const auto& data = editorOpenedObjects.at(index);
    Q_ASSERT(data.editor);
    if (FileBackedObject* obj = qobject_cast<FileBackedObject*>(data.obj)) {
        if (EditorBase* editor = qobject_cast<EditorBase*>(data.editor)) {
            if (!editor->isDirty())
                return true;

            // dirty case
            switchToEditor(index);
            auto reply = QMessageBox::question(this,
                                               tr("Save changes"),
                                               tr("You have unsaved changes in %1. Do you want to save them?").arg(obj->getFilePath()),
                                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            switch (reply) {
            default: Q_UNREACHABLE();
            case QMessageBox::Yes: {
                bool isGood = saveHelper(obj, data.editor);
                updateWindowTitle();
                if (!isGood)
                    return false;
            }break;
            case QMessageBox::No: {
                // Do nothing
            }break;
            case QMessageBox::Cancel: {
                return false;
            }
            }
        }
    }
    return true;
}

bool EditorWindow::closeAllCheck()
{
    for (int i = 0, n = editorOpenedObjects.size(); i < n; ++i) {
        if (!closeEditorCheck(i))
            return false;
    }
    return true;
}

bool EditorWindow::tryCloseAllMainContextObjects()
{
    if (tryCloseAllObjectsCommon(OriginContext::MainContext)) {
        mainCtx.clear();
        return true;
    }
    return false;
}

bool EditorWindow::tryCloseAllSideContextObjects()
{
    if (tryCloseAllObjectsCommon(OriginContext::SideContext)) {
        sideCtx.clear();
        return true;
    }
    return false;
}

void EditorWindow::closeEvent(QCloseEvent* event)
{
    if (!closeAllCheck()) {
        event->ignore();
        return;
    }
    event->accept();
}

void EditorWindow::saveRequested()
{
    if (currentOpenedObjectIndex == -1)
        return;

    Q_ASSERT(currentOpenedObjectIndex >= 0 && currentOpenedObjectIndex < editorOpenedObjects.size());
    EditorOpenedObjectData& data = editorOpenedObjects[currentOpenedObjectIndex];
    saveHelper(data.obj, data.editor);
    updateWindowTitle();
}

bool EditorWindow::saveHelper(ObjectBase* obj, QWidget* editor)
{
    Q_ASSERT(obj);
    if (editor) {
        if (EditorBase* editorPtr = qobject_cast<EditorBase*>(editor)) {
            editorPtr->saveToObjectRequested(obj);
        }
    }

    if (FileBackedObject* objPtr = qobject_cast<FileBackedObject*>(obj)) {
        bool isSaveSuccessful = objPtr->saveToFileStorage(this, mainCtx.getDirectory());
        if (isSaveSuccessful && editor) {
            if (EditorBase* editorPtr = qobject_cast<EditorBase*>(editor)) {
                editorPtr->clearDirty();
            }
        }
        return isSaveSuccessful;
    }

    return true;
}

void EditorWindow::changeDirectory(const QString& newDirectory)
{
    // step 1: close all editors if they are open
    if (!tryCloseAllObjectsCommon(OriginContext::MainContext))
        return;

    // step 2: refresh context
    mainCtx.setDirectory(newDirectory);

    // step 3: re-init the object list for main context
    populateObjectListTreeFromMainContext();

    mainRoot->setToolTip(0, mainCtx.getDirectory());

}

void EditorWindow::clipboardDumpRequested()
{
    MIMEDataObject* data = MIMEDataObject::dumpFromClipboard();
    if (!data) {
        QMessageBox::information(this, tr("No data"), tr("Clipboard is empty and no data is captured."));
        return;
    }

    addToSideContext(data);
}

void EditorWindow::addToSideContext(ObjectBase* obj, bool switchTo)
{
    sideCtx.addObject(obj);
    QTreeWidgetItem* item = new QTreeWidgetItem(sideRoot);
    updateObjectListItemForObject(item, obj);
    itemData.insert(item, ObjectListItemData(obj, nullptr, OriginContext::SideContext));
    sideRoot->setExpanded(true);
    if (switchTo) {
        objectListOpenEditorRequested(item);
    }
}

void EditorWindow::addToMainContext(IntrinsicObject* obj, bool switchTo)
{
    mainCtx.addObject(obj);
    Q_ASSERT(mainCtx.getObject(obj->getName()) == obj);
    QTreeWidgetItem* item = new QTreeWidgetItem(mainRoot);
    updateObjectListItemForObject(item, obj);
    itemData.insert(item, ObjectListItemData(obj, nullptr, OriginContext::MainContext));
    if (switchTo) {
        objectListOpenEditorRequested(item);
    }
}

void EditorWindow::closeSideContextObjectRequested(QTreeWidgetItem* item)
{
    Q_ASSERT(item->parent() == sideRoot);
    int indexOfItem = -1;
    QWidget* editorToSwitchTo = (currentOpenedObjectIndex >= 0)? editorOpenedObjects.at(currentOpenedObjectIndex).editor : nullptr;
    for (int i = 0, n = editorOpenedObjects.size(); i < n; ++i) {
        const auto& data = editorOpenedObjects.at(i);
        if (data.item == item) {
            // we will close this one
            int indexToSwitchTo = currentOpenedObjectIndex;
            if (!closeEditorCheck(i)) {
                // close request cancelled
                return;
            }
            switchToEditor(indexToSwitchTo);
            indexOfItem = i;
            break;
        }
    }
    if (indexOfItem != -1) {
        switchToEditor(-1);
        editorOpenedObjects.removeAt(indexOfItem);
        for (int i = 0, n = editorOpenedObjects.size(); i < n; ++i) {
            if (editorOpenedObjects.at(i).editor == editorToSwitchTo) {
                switchToEditor(i);
                break;
            }
        }
    }
    // now remove the item
    auto iter = itemData.find(item);
    Q_ASSERT(iter != itemData.end());
    ObjectBase* obj = iter.value().obj;
    sideCtx.releaseObject(obj);
    if (iter.value().editor) {
        obj->tearDownEditor(iter.value().editor);
    }
    itemData.erase(iter);
    delete item;
    delete obj;
}

void EditorWindow::dataDropped(const QMimeData* data)
{
    MIMEDataObject* ptr = new MIMEDataObject(*data);
    ptr->setName(tr("Drop"));
    ptr->setComment(tr("Time: %1").arg(QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate)));
    addToSideContext(ptr);
}

void EditorWindow::objectDropped(const QList<ObjectBase*>& vec)
{
    for (auto* ptr : vec) {
        ObjectBase* newObj = ptr->clone();
        // clear certain data
        newObj->setNameSpace(QStringList());
        addToSideContext(newObj);
    }
}

void EditorWindow::processDelayedStartupAction()
{
    mainCtx.setDirectory(startup.startDirectory);
    populateObjectListTreeFromMainContext();

    if (!startup.startFile.isEmpty()) {
        openFileRequestedOnPath(startup.startFile);
    }

    if (startup.isStartTaskSpecified) {
        if (!launchTask(startup.startTask))
            close();
    }
}

bool EditorWindow::launchTask(const ObjectBase::NamedReference& task)
{
    TaskObject::LaunchOptions options;
    ConfigurationData config;
    QHash<QString, QList<ObjectContext::AnonymousObjectReference>> inputs;
    QHash<QString, QVector<ExecuteWindow::OutputAction>> outputActions;
    ExecuteWindow* exec = ExecuteWindow::tryExecuteTask(task, options, config, inputs, outputActions, this);

    if (exec) {
        // TODO add signal connections
    }

    return exec != nullptr;
}

void EditorWindow::openFileRequested()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open File"), mainCtx.getDirectory(), tr("All files (*.*)"));
    if (path.isEmpty())
        return;

    openFileRequestedOnPath(path);
}

void EditorWindow::openFileRequestedOnPath(const QString& path)
{
    QFileInfo f(path);
    if (f.dir() == QDir(mainCtx.getDirectory())) {
        ObjectBase* obj = mainCtx.getObject(f.completeBaseName());
        if (obj != nullptr) {
            // this file is already opened in main context
            // just switch to it
            for (auto iter = itemData.begin(), iterEnd = itemData.end(); iter != iterEnd; ++iter) {
                if (iter.value().obj == obj) {
                    objectListOpenEditorRequested(iter.key());
                    return;
                }
            }
            qFatal("EditorWindow not showing item for object in main context!");
        }
    }

    FileBackedObject* obj = FileBackedObject::open(path, this);
    if (!obj) {
        return;
    }
    addToSideContext(obj);
}

void EditorWindow::createRequest_PlainText()
{
    PlainTextObject* obj = new PlainTextObject;
    obj->setName(tr("Unnamed"));
    addToSideContext(obj);
}

void EditorWindow::createRequest_IntrinsicObject()
{
    IntrinsicObjectCreationDialog* dialog = new IntrinsicObjectCreationDialog(mainCtx, this);
    connect(dialog, &QDialog::accepted, this, [=](){
        addToMainContext(dialog->getObject());
    });
    connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
    dialog->show();
}
