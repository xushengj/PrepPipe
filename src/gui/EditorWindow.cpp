#include "EditorWindow.h"
#include "ui_EditorWindow.h"

#include <QSettings>
#include <QMessageBox>
#include <QGridLayout>
#include <QCommonStyle>
#include <QFileDialog>

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

EditorWindow::EditorWindow(QString startDirectory, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::EditorWindow), mainCtx(startDirectory), sideCtx()
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
    ui->objectListTreeWidget->insertTopLevelItem(0, mainRoot);
    ui->objectListTreeWidget->insertTopLevelItem(1, sideRoot);
    populateObjectListTreeFromMainContext();

    connect(Settings::inst(), &Settings::settingChanged, this, &EditorWindow::settingChanged);

    ui->objectListTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu); // emit signal
    connect(ui->objectListTreeWidget, &QTreeWidget::itemClicked,                this, &EditorWindow::objectListItemClicked);
    connect(ui->objectListTreeWidget, &QTreeWidget::itemDoubleClicked,          this, &EditorWindow::objectListItemDoubleClicked);
    connect(ui->objectListTreeWidget, &QTreeWidget::customContextMenuRequested, this, &EditorWindow::objectListContextMenuRequested);

    connect(ui->actionChangeDirectory, &QAction::triggered, this, &EditorWindow::changeDirectoryRequested);
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::setObjectItem(QTreeWidgetItem* item, ObjectBase* obj)
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
        setObjectItem(item, objPtr);
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
}

void EditorWindow::objectListContextMenuRequested(const QPoint& pos)
{
    QTreeWidgetItem* item = ui->objectListTreeWidget->itemAt(pos);
    if (item == mainRoot) {
        QAction* cdAction = new QAction(tr("Change Directory..."));
        connect(cdAction, &QAction::triggered, ui->actionChangeDirectory, &QAction::trigger);
        QMenu menu(ui->objectListTreeWidget);
        menu.addAction(cdAction);
        menu.exec(ui->objectListTreeWidget->mapToGlobal(pos));
    }
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

bool EditorWindow::tryCloseAllMainContextObjects()
{
    // loop over all editor items and see which would be preserved and which would be closed
    decltype (editorOpenedObjects) tmpList;
    tmpList.reserve(editorOpenedObjects.size());
    int indexAfterClose = -1;
    for (int i = 0, n = editorOpenedObjects.size(); i < n; ++i) {
        const auto& data = editorOpenedObjects.at(i);
        if (data.origin == OriginContext::MainContext) {
            // we will close this one
            switchToEditor(i);
            if (!data.obj->editorOkayToClose(data.editor, this)) {
                // close request cancelled
                return false;
            }
        } else {
            if (indexAfterClose == -1) {
                indexAfterClose = i;
            }
            tmpList.push_back(data);
        }
    }
    switchToEditor(indexAfterClose);

    // now we are good to close all of them
    tmpList.swap(editorOpenedObjects);

    // remove items in object list tree
    for (auto iter = itemData.begin(); iter != itemData.end();) {
        QTreeWidgetItem* itemToDelete = nullptr;
        if (iter.key()->parent() == mainRoot) {
            itemToDelete = iter.key();
        }
        if (itemToDelete) {
            delete itemToDelete;
            iter = itemData.erase(iter);
        } else {
            ++iter;
        }
    }
    return true;
}

void EditorWindow::changeDirectory(const QString& newDirectory)
{
    // step 1: close all editors if they are open
    if (!tryCloseAllMainContextObjects())
        return;

    // step 2: refresh context
    mainCtx.setDirectory(newDirectory);

    // step 3: re-init the object list for main context
    populateObjectListTreeFromMainContext();
}
