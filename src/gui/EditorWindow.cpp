#include "EditorWindow.h"
#include "ui_EditorWindow.h"

#include <QSettings>

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

    initFromContext();
    initGUI();
    connect(Settings::inst(), &Settings::settingChanged, this, &EditorWindow::settingChanged);
}

EditorWindow::~EditorWindow()
{
    delete ui;
}

void EditorWindow::initFromContext()
{
    Q_ASSERT(namedObjectsByType.empty());
    Q_ASSERT(anonymousObjectsByType.empty());
    Q_ASSERT(openedGUIs.empty());

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
    auto setupTree = [=](
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

