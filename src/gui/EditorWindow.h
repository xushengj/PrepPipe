#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include "src/lib/ObjectContext.h"
#include "src/misc/Settings.h"

#include <QMainWindow>
#include <QLabel>
#include <QMap>
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui { class EditorWindow; }
QT_END_NAMESPACE

class EditorWindow : public QMainWindow
{
    Q_OBJECT

public:
    EditorWindow(ObjectContext* ctx, QWidget *parent = nullptr);
    virtual ~EditorWindow() override;
private:
    struct ObjectListItemData {
        ObjectBase* obj = nullptr;
        QWidget* editor = nullptr;
        ObjectListItemData() = default;
        ObjectListItemData(ObjectBase* objPtr, QWidget* editorPtr)
            : obj(objPtr), editor(editorPtr)
        {}
    };
    struct EditorOpenedObjectData {
        ObjectBase* obj = nullptr;
        QWidget* editor = nullptr;
        QTreeWidgetItem* item = nullptr;

        EditorOpenedObjectData() = default;
        EditorOpenedObjectData(ObjectBase* objPtr, QWidget* editorPtr, QTreeWidgetItem* itemPtr)
            : obj(objPtr), editor(editorPtr), item(itemPtr)
        {}
    };

    void initFromContext();
    void initGUI();

    void refreshObjectListGUI();

    void showObjectEditor(ObjectBase* obj, QWidget* editor, QTreeWidgetItem* item);

    virtual void contextMenuEvent(QContextMenuEvent *event) override; // right click menu

private slots:
    void settingChanged(const QStringList& keyList);

    void objectListContextMenuRequested(const QPoint& pos);
    void objectListItemClicked(QTreeWidgetItem* item, int column);
    void objectListItemDoubleClicked(QTreeWidgetItem* item, int column);

    void objectListOpenEditorRequested(QTreeWidgetItem* item);
private:
    Ui::EditorWindow *ui;
    ObjectContext* ctx;

    QTreeWidgetItem* namedRoot;
    QTreeWidgetItem* anonymousRoot;
    QMap<ObjectBase::ObjectType, QTreeWidgetItem*> namedObjectTypeItem;
    QMap<ObjectBase::ObjectType, QTreeWidgetItem*> anonymousObjectTypeItem;
    QHash<ObjectBase*, QTreeWidgetItem*> namedObjectItems;
    QHash<ObjectBase*, QTreeWidgetItem*> anonymousObjectItems;
    QMap<ObjectBase::ObjectType, QList<ObjectBase*>> namedObjectsByType;
    QMap<ObjectBase::ObjectType, QList<ObjectBase*>> anonymousObjectsByType;

    QHash<QTreeWidgetItem*, ObjectListItemData> itemData;
    QList<EditorOpenedObjectData> editorOpenedObjects;
    int currentOpenedObjectIndex = -1;
};
#endif // EDITORWINDOW_H
