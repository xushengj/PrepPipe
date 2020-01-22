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
    EditorWindow(QString startDirectory, QWidget *parent = nullptr);
    virtual ~EditorWindow() override;
private:
    enum class OriginContext : int {
        MainContext = 0,
        SideContext = 1
    };
    struct ObjectListItemData {
        ObjectBase* obj = nullptr;
        QWidget* editor = nullptr;
        OriginContext origin = OriginContext::MainContext;
        ObjectListItemData() = default;
        ObjectListItemData(ObjectBase* objPtr, QWidget* editorPtr, OriginContext originValue)
            : obj(objPtr), editor(editorPtr), origin(originValue)
        {}
    };
    struct EditorOpenedObjectData {
        ObjectBase* obj = nullptr;
        QWidget* editor = nullptr;
        QTreeWidgetItem* item = nullptr;
        OriginContext origin = OriginContext::MainContext;

        EditorOpenedObjectData() = default;
        EditorOpenedObjectData(ObjectBase* objPtr, QWidget* editorPtr, QTreeWidgetItem* itemPtr, OriginContext originValue)
            : obj(objPtr), editor(editorPtr), item(itemPtr), origin(originValue)
        {}
    };

    void initFromContext();
    void initGUI();

    void populateObjectListTreeFromMainContext();
    static void setObjectItem(QTreeWidgetItem* item, ObjectBase* obj);

    void showObjectEditor(ObjectBase* obj, QWidget* editor, QTreeWidgetItem* item, OriginContext origin);
    void switchToEditor(int index);
    void changeDirectory(const QString& newDirectory);
    bool tryCloseAllMainContextObjects();

    virtual void contextMenuEvent(QContextMenuEvent *event) override; // right click menu

private slots:
    void settingChanged(const QStringList& keyList);

    void objectListContextMenuRequested(const QPoint& pos);
    void objectListItemClicked(QTreeWidgetItem* item, int column);
    void objectListItemDoubleClicked(QTreeWidgetItem* item, int column);

    void objectListOpenEditorRequested(QTreeWidgetItem* item);

    void changeDirectoryRequested();
private:
    Ui::EditorWindow *ui;
    ObjectContext mainCtx; // what's in working directory
    ObjectContext sideCtx; // what's "floating"

    QTreeWidgetItem* mainRoot = nullptr;
    QTreeWidgetItem* sideRoot = nullptr;

    QHash<QTreeWidgetItem*, ObjectListItemData> itemData;
    QList<EditorOpenedObjectData> editorOpenedObjects;
    int currentOpenedObjectIndex = -1;
};
#endif // EDITORWINDOW_H
