#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include "src/lib/ObjectContext.h"
#include "src/lib/TaskObject.h"
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
    EditorWindow(QString startDirectory, QString startTask, QStringList presets, QWidget *parent = nullptr);
    virtual ~EditorWindow() override;

    const ObjectContext& getMainContext() const {return mainCtx;}

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

    bool tryCloseAllObjectsCommon(OriginContext origin);

    void addToSideContext(ObjectBase* obj);

    virtual void contextMenuEvent(QContextMenuEvent *event) override; // right click menu

    // convenient version
    bool launchTask(const ObjectBase::NamedReference &task);

    // for drag/drop
    bool getObjectReference(QTreeWidgetItem* item, ObjectContext::AnonymousObjectReference& ref);

private slots:
    void settingChanged(const QStringList& keyList);

    void objectListContextMenuRequested(const QPoint& pos);
    void objectListItemClicked(QTreeWidgetItem* item, int column);
    void objectListItemDoubleClicked(QTreeWidgetItem* item, int column);

    void objectListOpenEditorRequested(QTreeWidgetItem* item);

    void changeDirectoryRequested();
    void clipboardDumpRequested();
    void openFileRequested();
    void dataDropped(const QMimeData* data);
    void objectDropped(const QList<ObjectBase*>& vec);

    void closeSideContextObjectRequested(QTreeWidgetItem* item);
    bool tryCloseAllSideContextObjects();

    void processDelayedStartupAction();

private:
    struct StartupInfo {
        QString startDirectory;
        QHash<QString, QString> presets;
        ObjectBase::NamedReference startTask;
        bool isStartTaskSpecified;
    } startup;

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
