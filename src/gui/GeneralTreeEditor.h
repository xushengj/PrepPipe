#ifndef GENERALTREEEDITOR_H
#define GENERALTREEEDITOR_H

#include "src/gui/EditorBase.h"
#include "src/lib/Tree/Tree.h"

#include <QtGlobal>
#include <QWidget>
#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include <QTableView>
#include <QTreeView>

#include <deque>
#include <functional>

namespace Ui {
class GeneralTreeEditor;
}

class GeneralTreeModel;
class GeneralTreeNodeModel;
class GeneralTreeEditor : public EditorBase
{
    Q_OBJECT

public:
    // TODO currently is just a viewer
    explicit GeneralTreeEditor(QWidget *parent = nullptr);
    ~GeneralTreeEditor();

    void setData(const Tree& data);


public slots:
    void setReadOnly(bool ro);

public:
    virtual void saveToObjectRequested(ObjectBase* obj) override;

private:
    Ui::GeneralTreeEditor *ui;
    GeneralTreeModel* model = nullptr;
    bool isReadOnly = false;
};

class GeneralTreeNodeModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    // create an invalid one
    GeneralTreeNodeModel();

    // create a valid one
    GeneralTreeNodeModel(QStringList& keyList, QStringList& valueList);

    ~GeneralTreeNodeModel();

    enum TableColumn {
        COL_KEY    = 0,
        COL_VALUE  = 1,
        NUM_COL
    };

public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex& index = QModelIndex()) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

public slots:
    void setReadOnly(bool ro);

private:
    QStringList& keys;
    QStringList& values;
    bool isReadOnly = false;
    const bool isInvalid = false;

    QStringList dummyVal;
};

class GeneralTreeModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    GeneralTreeModel(QObject *parent = nullptr);
    ~GeneralTreeModel();

    void setProvenanceColorCallback(std::function<QColor(quint64)> cb) {
        getProvenanceColorCallback = cb;
    }

    void finishInit(QTreeView* tree, QTableView* node);

    void setData(const Tree& data, QVector<int> provenanceColorIndexVec = QVector<int>());

    static Qt::ItemDataRole getProvenanceColorRule() {
        return Qt::BackgroundRole;
    }

public slots:
    void setReadOnly(bool ro);
    void provenanceColorUpdated();

public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    virtual QModelIndex parent(const QModelIndex &index) const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;

private slots:
    void currentNodeChanged();

private:
    struct TreeNodeRepresentation {
        // data same as in Tree::Node
        QString typeName;
        QStringList keyList;
        QStringList valueList;

        // use (persistent, one time generated, pointer like) id that is never reused
        quint64 selfID = 0;
        quint64 parentID = 0; // should not be used on root node
        std::vector<quint64> childIDList;

        // misc
        int provenanceColorIndex = -1;

        // a numerical identifier attached to displayed name of node
        quint64 displayOrderIndex = 0;
    };

private:
    QString getNodeDisplayString(const TreeNodeRepresentation& node) const;

    quint64 populateFromTree(const Tree& data, const QVector<int>& provenanceColorIndexVec, int curIndex, quint64 parentIndex);
    quint64 updateDisplayOrderIndex(quint64 curNode, quint64 curNodeDisplayOrder);
    QModelIndex getModelIndexFromInternalID(quint64 id) const;

private:
    QTreeView* treeView = nullptr;
    QTableView* nodeTableView = nullptr;
    std::deque<TreeNodeRepresentation> nodes;
    std::function<QColor(int)> getProvenanceColorCallback;
    quint64 rootNodeIndex = 0;
    quint64 activeNodeIndex = 0;
    bool isReadOnly = false;
};


#endif // GENERALTREEEDITOR_H
