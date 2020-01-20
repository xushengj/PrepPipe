#include "EditorWindow.h"
#include "ui_EditorWindow.h"

#include <QSettings>
#include <QMessageBox>
#include <QGridLayout>

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

EditorWindow::EditorWindow(ObjectContext *ctxptr, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::EditorWindow), ctx(ctxptr)
{
    ui->setupUi(this);
    Q_ASSERT(ctxptr);
    setContextMenuPolicy(Qt::DefaultContextMenu); // call contextMenuEvent()

    initFromContext();
    initGUI();
    connect(Settings::inst(), &Settings::settingChanged, this, &EditorWindow::settingChanged);

    ui->objectListTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu); // emit signal
    connect(ui->objectListTreeWidget, &QTreeWidget::itemClicked,                this, &EditorWindow::objectListItemClicked);
    connect(ui->objectListTreeWidget, &QTreeWidget::itemDoubleClicked,          this, &EditorWindow::objectListItemDoubleClicked);
    connect(ui->objectListTreeWidget, &QTreeWidget::customContextMenuRequested, this, &EditorWindow::objectListContextMenuRequested);
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::initFromContext()
{
    Q_ASSERT(namedObjectsByType.empty());
    Q_ASSERT(anonymousObjectsByType.empty());

    auto addToDict = [](decltype(namedObjectsByType)& dict, ObjectBase* obj) -> void {
        dict[obj->getType()].push_back(obj);
    };

    for (auto objPtr : *ctx) {
        if (objPtr->getName().isEmpty()) {
            addToDict(anonymousObjectsByType, objPtr);
        } else {
            addToDict(namedObjectsByType, objPtr);
        }
    }
}

void EditorWindow::initGUI()
{
    ui->placeholderLabel->setText(getPlaceHolderText());
    ui->currentObjectComboBox->addItem(tr("(No object is opened)"));
    ui->currentObjectComboBox->setEnabled(false);

    refreshObjectListGUI();
}

void EditorWindow::refreshObjectListGUI()
{
    ui->objectListTreeWidget->clear();
    namedRoot = nullptr;
    anonymousRoot = nullptr;
    namedObjectTypeItem.clear();
    anonymousObjectTypeItem.clear();
    namedObjectItems.clear();
    anonymousObjectItems.clear();
    itemData.clear();
    auto setupTree = [this](
            decltype(namedRoot) root,
            decltype(namedObjectTypeItem)& typeItem,
            decltype(namedObjectItems)& objectItems,
            const decltype(namedObjectsByType)& objByType) -> void {
        for (auto iter = objByType.begin(), iterEnd = objByType.end(); iter != iterEnd; ++iter) {
            ObjectBase::ObjectType ty = iter.key();
            QTreeWidgetItem* tyItem = new QTreeWidgetItem(root,QStringList(ObjectBase::getTypeDisplayName(ty)));
            tyItem->setIcon(0, ObjectBase::getTypeDisplayIcon(ty));
            typeItem.insert(ty, tyItem);
            for (auto objPtr : iter.value()) {
                QTreeWidgetItem* objItem = new QTreeWidgetItem(tyItem, QStringList(objPtr->getName()));
                objectItems.insert(objPtr, objItem);
                itemData.insert(objItem, ObjectListItemData(objPtr, nullptr));
            }
        }
    };
    int rowCnt = 0;
    if (!namedObjectsByType.isEmpty()) {
        namedRoot = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Named")));
        ui->objectListTreeWidget->insertTopLevelItem(rowCnt, namedRoot);
        rowCnt += 1;
        setupTree(namedRoot, namedObjectTypeItem, namedObjectItems, namedObjectsByType);
    }
    if (!anonymousObjectsByType.isEmpty()) {
        anonymousRoot = new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(nullptr), QStringList(tr("Anonymous")));
        ui->objectListTreeWidget->insertTopLevelItem(rowCnt, anonymousRoot);
        rowCnt += 1;
        setupTree(anonymousRoot, anonymousObjectTypeItem, anonymousObjectItems, anonymousObjectsByType);
    }
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
        showObjectEditor(data.obj, data.editor, item);
    } else {
        QMessageBox::information(this, tr("Feature not implemented"), tr("Sorry, editing or viewing this object is not supported."));
    }
}

void EditorWindow::showObjectEditor(ObjectBase* obj, QWidget* editor, QTreeWidgetItem* item)
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
        editorOpenedObjects.push_back(EditorOpenedObjectData(obj, editor, item));
    }

    // now try to actually switch the editor
    // do nothing if current one is the desired
    if (index == currentOpenedObjectIndex)
        return;

    QWidget* previousWidget = (currentOpenedObjectIndex != -1)? editorOpenedObjects.at(currentOpenedObjectIndex).editor : ui->placeholderLabel;
    currentOpenedObjectIndex = index;
    ui->placeholderWidget->layout()->removeWidget(previousWidget);
    previousWidget->hide();
    qobject_cast<QGridLayout*>(ui->placeholderWidget->layout())->addWidget(editor, 0, 0);
    editor->show();
}

void EditorWindow::objectListContextMenuRequested(const QPoint& pos)
{
    // TODO
    Q_UNUSED(pos)
}

void EditorWindow::contextMenuEvent(QContextMenuEvent *event)
{
    // TODO
    event->accept();
}
