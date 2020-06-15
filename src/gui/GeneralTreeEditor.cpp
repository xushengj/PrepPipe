#include "GeneralTreeEditor.h"
#include "ui_GeneralTreeEditor.h"

GeneralTreeEditor::GeneralTreeEditor(QWidget *parent) :
    EditorBase(parent),
    ui(new Ui::GeneralTreeEditor),
    model(new GeneralTreeModel)
{
    ui->setupUi(this);
    ui->treeView->setHeaderHidden(true);
    ui->nodeTableView->horizontalHeader()->setStretchLastSection(true);
    model->finishInit(ui->treeView, ui->nodeTableView);
}

GeneralTreeEditor::~GeneralTreeEditor()
{
    delete ui;
}

void GeneralTreeEditor::setData(const Tree& data)
{
    model->setData(data);
}

void GeneralTreeEditor::setReadOnly(bool ro)
{
    model->setReadOnly(ro);
}

void GeneralTreeEditor::saveToObjectRequested(ObjectBase* obj)
{
    // TODO
    Q_UNUSED(obj)
}

// ----------------------------------------------------------------------------

GeneralTreeNodeModel::GeneralTreeNodeModel(QStringList& keyList, QStringList& valueList)
    : keys(keyList),
      values(valueList)
{

}

GeneralTreeNodeModel::GeneralTreeNodeModel()
    : keys(dummyVal),
      values(dummyVal),
      isInvalid(true)
{

}

GeneralTreeNodeModel::~GeneralTreeNodeModel()
{

}

void GeneralTreeNodeModel::setReadOnly(bool ro)
{
    if (isInvalid)
        return;

    beginResetModel();
    isReadOnly = ro;
    endResetModel();
}

int GeneralTreeNodeModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return keys.size();
}

int GeneralTreeNodeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return NUM_COL;
}

QVariant GeneralTreeNodeModel::data(const QModelIndex &index, int role) const
{
    int rowIndex = index.row();
    if (rowIndex < 0 || rowIndex >= keys.size())
        return QVariant();

    switch (index.column()) {
    default: return QVariant();
    case COL_KEY: {
        switch (role) {
        default: return QVariant();
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(keys.at(rowIndex));
        }
    }break;
    case COL_VALUE: {
        switch (role) {
        default: return QVariant();
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(values.at(rowIndex));
        }
    }break;
    }
}

QVariant GeneralTreeNodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // no headers in vertical direction
    if (orientation == Qt::Vertical || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    default: return QVariant();
    case COL_KEY: {
        return QVariant(tr("Name"));
    }break;
    case COL_VALUE: {
        return QVariant(tr("Value"));
    }break;
    }
}

Qt::ItemFlags GeneralTreeNodeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    if (isReadOnly) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    } else {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }
}

bool GeneralTreeNodeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (isInvalid)
        return false;

    if (!index.isValid() || isReadOnly || role != Qt::EditRole)
        return false;

    int rowIndex = index.row();
    if (rowIndex < 0 || rowIndex >= keys.size())
        return false;

    Q_ASSERT(keys.size() == values.size());

    QString data = value.toString();

    switch (index.column()) {
    default: return false;
    case COL_KEY: {
        keys[rowIndex] = data;
    }break;
    case COL_VALUE: {
        values[rowIndex] = data;
    }break;
    }

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
    return true;
}

// ----------------------------------------------------------------------------

GeneralTreeModel::GeneralTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    // create a dummy one as invalid
    nodes.push_back(TreeNodeRepresentation());
}

GeneralTreeModel::~GeneralTreeModel()
{

}

void GeneralTreeModel::finishInit(QTreeView* tree, QTableView* node)
{
    treeView = tree;
    nodeTableView = node;
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setModel(this);
    if (!tree->selectionModel()) {
        tree->setSelectionModel(new QItemSelectionModel(this));
    }
    connect(tree->selectionModel(), &QItemSelectionModel::currentChanged, this, &GeneralTreeModel::currentNodeChanged);
}

void GeneralTreeModel::setData(const Tree& data, QVector<int> provenanceColorIndexVec)
{
    beginResetModel();

    // delete node model
    {
        activeNodeIndex = 0;
        QAbstractItemModel* oldModel = nodeTableView->model();
        nodeTableView->setModel(new GeneralTreeNodeModel);
        delete oldModel;
    }

    nodes.clear();
    nodes.push_back(TreeNodeRepresentation());

    if (data.isEmpty()) {
        // empty tree; create a dummy node and done
        rootNodeIndex = nodes.size();
        nodes.push_back(TreeNodeRepresentation());
        return;
    }

    rootNodeIndex = populateFromTree(data, provenanceColorIndexVec, /* root node in tree */0, /* parent index of root node is invalid */0);
    updateDisplayOrderIndex(rootNodeIndex, 0);

    endResetModel();
    treeView->expandAll();
}

quint64 GeneralTreeModel::populateFromTree(const Tree& data, const QVector<int>& provenanceColorIndexVec, int curIndex, quint64 parentIndex)
{
    quint64 curNodeIndex = nodes.size();
    const auto& curNode = data.getNode(curIndex);
    TreeNodeRepresentation newNode;
    newNode.typeName = curNode.typeName;
    newNode.keyList = curNode.keyList;
    newNode.valueList = curNode.valueList;
    newNode.selfID = curNodeIndex;
    newNode.parentID = parentIndex;

    if (!provenanceColorIndexVec.isEmpty()) {
        newNode.provenanceColorIndex = provenanceColorIndexVec.at(curIndex);
    }

    nodes.push_back(newNode);

    std::size_t numChild = static_cast<std::size_t>(curNode.offsetToChildren.size());
    decltype(newNode.childIDList) children;
    children.reserve(numChild);
    for (std::size_t i = 0; i < numChild; ++i) {
        quint64 curChild = populateFromTree(data, provenanceColorIndexVec, curIndex + curNode.offsetToChildren.at(static_cast<int>(i)), curNodeIndex);
        children.push_back(curChild);
    }

    nodes[curNodeIndex].childIDList.swap(children);
    return curNodeIndex;
}

quint64 GeneralTreeModel::updateDisplayOrderIndex(quint64 curNode, quint64 curNodeDisplayOrder)
{
    auto& node = nodes.at(curNode);
    node.displayOrderIndex = curNodeDisplayOrder;
    quint64 nextOrderIndex = curNodeDisplayOrder + 1;
    for (auto child : node.childIDList) {
        nextOrderIndex = updateDisplayOrderIndex(child, nextOrderIndex);
    }
    return nextOrderIndex;
}

void GeneralTreeModel::currentNodeChanged()
{
    QModelIndex curIndex = treeView->selectionModel()->currentIndex();

    // we delete the old model anyway
    GeneralTreeNodeModel* newModel = nullptr;
    if (!curIndex.isValid()) {
        newModel = new GeneralTreeNodeModel;
        activeNodeIndex = 0;
    } else {
        auto& node = nodes.at(curIndex.internalId());
        newModel = new GeneralTreeNodeModel(node.keyList, node.valueList);
        quint64 id = curIndex.internalId();
        activeNodeIndex = id;
        connect(newModel, &QAbstractItemModel::dataChanged, this, [=](){
            auto index = getModelIndexFromInternalID(id);
            emit dataChanged(index, index);
        });
        newModel->setReadOnly(isReadOnly);
    }
    QAbstractItemModel* oldModel = nodeTableView->model();
    nodeTableView->setModel(newModel);
    delete oldModel;
}

QModelIndex GeneralTreeModel::getModelIndexFromInternalID(quint64 id) const
{
    if (id >= nodes.size()) {
        return QModelIndex();
    }

    if (id == rootNodeIndex) {
        // this is the root node
        // because we only have one root node, the root node must be row 0
        return createIndex(0, 0, rootNodeIndex);
    }

    // okay, there is a parent node
    quint64 parentID = nodes.at(id).parentID;

    // determine which row it is in parent's child list
    const auto& parentNode = nodes.at(parentID);
    int row = -1;

    for (std::size_t i = 0, n = parentNode.childIDList.size(); i < n; ++i) {
        if (parentNode.childIDList.at(i) == id) {
            row = static_cast<int>(i);
            break;
        }
    }
    Q_ASSERT(row >= 0);

    return createIndex(row, 0, id);
}

void GeneralTreeModel::provenanceColorUpdated()
{
    beginResetModel();
    endResetModel();
}

void GeneralTreeModel::setReadOnly(bool ro)
{
    isReadOnly = ro;
    if (auto* model = nodeTableView->model()) {
        if (GeneralTreeNodeModel* nodeModel = qobject_cast<GeneralTreeNodeModel*>(model)) {
            nodeModel->setReadOnly(ro);
        }
    }
}

QModelIndex GeneralTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return QModelIndex();

    if (!parent.isValid()) {
        if (row != 0) {
            return QModelIndex();
        }
        // this is the root node
        return createIndex(0, 0, rootNodeIndex);
    }

    quint64 parentID = parent.internalId();
    if (parentID > nodes.size())
        return QModelIndex();

    const auto& parentNode = nodes.at(parentID);
    if (row < 0 || row >= static_cast<int>(parentNode.childIDList.size())) {
        return QModelIndex();
    }

    return createIndex(row, 0, parentNode.childIDList.at(row));
}

QModelIndex GeneralTreeModel::parent(const QModelIndex &index) const
{
    quint64 id = index.internalId();
    if (!index.isValid() || id >= nodes.size())
        return QModelIndex();

    if (id == rootNodeIndex) {
        // this is the root node
        // root node don't have parents
        return QModelIndex();
    }

    quint64 parentID = nodes.at(id).parentID;

    return getModelIndexFromInternalID(parentID);
}

int GeneralTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        // we only have 1 root node
        return 1;
    }

    quint64 id = parent.internalId();
    if (id >= nodes.size()) {
        return 0;
    }

    return static_cast<int>(nodes.at(id).childIDList.size());
}

int GeneralTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant GeneralTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0) {
        return QVariant();
    }

    quint64 id = index.internalId();
    if (id >= nodes.size())
        return QVariant();

    const auto& node = nodes.at(id);

    if (role == getProvenanceColorRule()) {
        int colorIndex = node.provenanceColorIndex;
        if (getProvenanceColorCallback) {
            return QVariant(getProvenanceColorCallback(colorIndex));
        } else {
            return QVariant();
        }
    }
    switch (role) {
    default: return QVariant();
    case Qt::EditRole: return QVariant(node.typeName);
    case Qt::DisplayRole: return QVariant(getNodeDisplayString(node));
    }
}

bool GeneralTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    // TODO
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    return false;
}

QString GeneralTreeModel::getNodeDisplayString(const TreeNodeRepresentation& node) const
{
    QString str = QString("[%1] %2").arg(QString::number(node.displayOrderIndex), node.typeName);
    if (!node.valueList.isEmpty()) {
        QString aux_str;
        if (node.valueList.size() == 1) {
            aux_str = node.valueList.front();
        } else {
            for (int i = 0, n = node.valueList.size(); i < n; ++i) {
                if (i > 0) {
                    aux_str.append(", ");
                }
                aux_str.append(node.keyList.at(i));
                aux_str.append("=\"");
                aux_str.append(node.valueList.at(i));
                aux_str.append('"');
            }
        }
        str.append(" (");
        str.append(aux_str);
        str.append(')');
    }
    return str;
}
