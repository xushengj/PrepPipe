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
    ~EditorWindow();
private:
    void initFromContext();
    void initGUI();

    void refreshObjectListGUI();
private slots:
    void settingChanged(const QStringList& keyList);
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

    QVector<std::pair<ObjectBase*,QWidget*>> openedGUIs;
};
#endif // EDITORWINDOW_H
