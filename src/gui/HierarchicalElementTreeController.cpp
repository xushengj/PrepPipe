#include "src/gui/HierarchicalElementTreeController.h"
#include "src/gui/PPMIMETypes.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QMimeData>

#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>

HierarchicalElementTreeControllerObject::HierarchicalElementTreeControllerObject()
    : QAbstractItemModel(nullptr), uuid(QUuid::createUuid())
{}

void HierarchicalElementTreeControllerObject::init(QTreeView* treeWidgetArg, QStackedWidget* stackedWidgetArg, BidirStringList& nameListArg, GraphData *graphDataArg, const char* controllerTypeNameArg, std::size_t controllerTypeHashArg)
{
    treeWidget = treeWidgetArg;
    stackedWidget = stackedWidgetArg;
    nameList = &nameListArg;
    graphData = graphDataArg;
    controllerTypeName = controllerTypeNameArg;
    controllerTypeHash = controllerTypeHashArg;

    // initialize model tree data with two reserved nodes
    {
        ModelTreeNode tmp;
        tmp.elementNameIndex = 0;
        tmp.parentNodeIndex = 0;
        modelTreeData.push_back(tmp);
        tmp.elementNameIndex = 1;
        tmp.parentNodeIndex = 1;
        modelTreeData.push_back(tmp);
    }

    treeWidget->setModel(this);
    treeWidget->setHeaderHidden(true);
    treeWidget->expandAll();
    treeWidget->setItemsExpandable(false);
    treeWidget->setRootIsDecorated(false);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget->setDragEnabled(true);
    treeWidget->viewport()->setAcceptDrops(true);
    treeWidget->setDropIndicatorShown(true);
    treeWidget->setDefaultDropAction(Qt::MoveAction);

    // https://stackoverflow.com/questions/16018974/qtreeview-remove-decoration-expand-button-for-all-items
    // treeWidget->setStyleSheet( "QTreeView::branch {  border-image: url(none.png); }" );

    connect(treeWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, &HierarchicalElementTreeControllerObject::currentElementChangeHandler);

    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(treeWidget, &QWidget::customContextMenuRequested, this, &HierarchicalElementTreeControllerObject::listWidgetContextMenuEventHandler);
}

void HierarchicalElementTreeControllerObject::dataChanged()
{
    rebuildModelTreeData();
}

void HierarchicalElementTreeControllerObject::currentElementChangeHandler()
{
    if (currentElementChangeCB) {
        const auto& index = treeWidget->selectionModel()->currentIndex();
        if (index.isValid()) {
            const auto& data = modelTreeData.at(index.internalId());
            currentElementChangeCB(data.elementNameIndex);
        }
    } else {
        qWarning() << "Current element changed but no callback is installed; change event ignored";
    }
}

void HierarchicalElementTreeControllerObject::listWidgetContextMenuEventHandler(const QPoint& p)
{
    if (listWidgetContextMenuEventCB) {
        listWidgetContextMenuEventCB(p);
    } else {
        qWarning() << "List widget context menu requested but no callback is installed; request ignored";
    }
}

void HierarchicalElementTreeControllerObject::tryGoToElement(const QString& elementName)
{
    if (gotoElementCB) {
        gotoElementCB(elementName);
    }
}

void HierarchicalElementTreeControllerObject::tryCreateElement(const QString& elementName)
{
    if (createElementCB) {
        createElementCB(elementName);
    } else {
        qWarning() << "Element creation requested but no callback installed; request ignored";
    }
}

void HierarchicalElementTreeControllerObject::rebuildModelTreeData()
{
    beginResetModel();
    modelTreeData.clear();
    {
        ModelTreeNode tmp;
        tmp.elementNameIndex = 0;
        tmp.parentNodeIndex = 0;
        modelTreeData.push_back(tmp);
        tmp.elementNameIndex = 1;
        tmp.parentNodeIndex = 1;
        modelTreeData.push_back(tmp);
    }
    std::vector<bool> referencedNodes(nameList->size(), false);
    std::deque<indextype> nodesToExpand; // node / reference index
    auto processNameVecImpl = [&](const QStringList& list, indextype parent, std::function<void(indextype, indextype)> unvisitedNodeOp) -> ContiguousIndexVector<indextype> {
        ContiguousIndexVector<indextype> result;
        result.reserve(list.size());
        for (const QString& str : list) {
            indextype elementIndex = nameList->indexOf(str);
            Q_ASSERT(elementIndex >= 0 && elementIndex < nameList->size());
            // create a new reference
            ModelTreeNode child;
            child.elementNameIndex = elementIndex;
            child.parentNodeIndex = parent;
            indextype childNodeIndex = modelTreeData.size();
            modelTreeData.push_back(child);
            result.push_back(childNodeIndex);
            if (!referencedNodes[elementIndex]) {
                nodesToExpand.push_back(childNodeIndex);
                if (unvisitedNodeOp) {
                    unvisitedNodeOp(elementIndex, childNodeIndex);
                }
            }
            referencedNodes[elementIndex] = true;
        }
        return result;
    };

    auto processNameVec = [&](const QStringList& list, indextype parent) -> ContiguousIndexVector<indextype> {
        return processNameVecImpl(list, parent, std::function<void(indextype, indextype)>());
    };

    modelTreeData[INDEX_ROOT_TOPNODES].childNodeIndices = processNameVec(graphData->getTopElementList(), INDEX_ROOT_TOPNODES);

    while(!nodesToExpand.empty()) {
        indextype curNode = nodesToExpand.front();
        nodesToExpand.pop_front();
        indextype elementNameIndex = modelTreeData.at(curNode).elementNameIndex;
        auto resultNodeVec = processNameVec(graphData->getChildElementList(nameList->at(elementNameIndex)), curNode);
        modelTreeData[curNode].childNodeIndices = resultNodeVec;
    }

    // ok, now all nodes reachable from root should have been added
    // next step is to add all unreachable nodes
    // we need to find the minimal set of unreachable nodes so that if they are reachable top-level nodes, then all remaining nodes are also reachable
    // we do this using the following algorith:
    // while (set of unvisited unreachable node not empty) {
    //     pick a node as a starting point
    //     try to find its parent node
    //         if it is a child of more than one node, pick any one of them (preferably the one with more children) and continue parent finding.
    //     if a loop is detected:
    //         add the starting node
    //     else (i.e., no loop):
    //         add the top-most node
    //     drop all nodes reachable from the selected node
    // }
    // step 1: find all nodes that are not directly unreachable
    // since we may also have loops in those unreferenced nodes, just expand
    std::unordered_set<indextype> unvisitedNodes;
    for (std::size_t i = 0, n = referencedNodes.size(); i < n; ++i) {
        if (!referencedNodes[i]) {
            unvisitedNodes.insert(i);
        }
    }
    std::unordered_map<indextype, indextype> childSizes; // for each element, how many children (in the unvisited set of nodes) do they have
    std::unordered_map<indextype, indextype> parentMap; // for each child, what's the current parent with the most number of child
    for (indextype index : unvisitedNodes) {
        QStringList childList = graphData->getChildElementList(nameList->at(index));
        std::vector<indextype> unvisitedChild;
        for (const QString& child : childList) {
            indextype idx = nameList->indexOf(child);
            if (unvisitedNodes.find(idx) == unvisitedNodes.end()) {
                continue;
            }
            unvisitedChild.push_back(idx);
        }
        if (unvisitedChild.empty()) {
            continue;
        }
        indextype size = unvisitedChild.size();
        childSizes[index] = size;
        for (indextype child : unvisitedChild) {
            indextype pastCount = 0;
            {
                auto iter = parentMap.find(child);
                if (iter != parentMap.end()) {
                    auto iterParentSize = childSizes.find(iter->second);
                    Q_ASSERT(iterParentSize != childSizes.end());
                    pastCount = iterParentSize->second;
                }
            }
            if (pastCount < size) {
                parentMap[child] = index;
            }
        }
    }
    struct PendingTopDisabledNodeRecord {
        ContiguousIndexVector<indextype> childNodeIndices;
        indextype elementIndex = 0;
    };
    std::vector<PendingTopDisabledNodeRecord> pendingNodes;

    auto dropVisitedNode = [&](indextype elementIndex, indextype nodeIndex) -> void {
        Q_UNUSED(nodeIndex);
        auto iter = unvisitedNodes.find(elementIndex);
        Q_ASSERT(iter != unvisitedNodes.end());
        unvisitedNodes.erase(iter);
    };

    while (!unvisitedNodes.empty()) {
        indextype startIndex = *unvisitedNodes.begin();
        indextype curTop = startIndex;
        indextype smallestInLoop = startIndex;
        auto iter = parentMap.find(startIndex);
        while (iter != parentMap.end()) {
            curTop = iter->second;
            if (curTop == startIndex) {
                curTop = smallestInLoop;
                break;
            }
            if (smallestInLoop > curTop) {
                smallestInLoop = curTop;
            }
            iter = parentMap.find(curTop);
        }
        // curTop is selected as one node
        {
            auto iter = unvisitedNodes.find(curTop);
            Q_ASSERT(iter != unvisitedNodes.end());
            unvisitedNodes.erase(iter);
        }
        referencedNodes[curTop] = true;
        PendingTopDisabledNodeRecord tmp;
        tmp.elementIndex = curTop;
        tmp.childNodeIndices = processNameVecImpl(graphData->getChildElementList(nameList->at(curTop)), 0, dropVisitedNode); // we will correct them later on
        pendingNodes.push_back(tmp);
        while(!nodesToExpand.empty()) {
            indextype curNode = nodesToExpand.front();
            nodesToExpand.pop_front();
            indextype elementNameIndex = modelTreeData.at(curNode).elementNameIndex;
            auto resultNodeVec = processNameVecImpl(graphData->getChildElementList(nameList->at(elementNameIndex)), curNode, dropVisitedNode);
            modelTreeData[curNode].childNodeIndices = resultNodeVec;
        }
    }
    // done; now all nodes are covered
    // add top-level disabled nodes
    for (PendingTopDisabledNodeRecord& pendingNodeData : pendingNodes) {
        indextype disabledNodeIndex = modelTreeData.size();
        for (indextype child : pendingNodeData.childNodeIndices) {
            Q_ASSERT(child >= NUM_RESERVED_NODES && child <= modelTreeData.size());
            modelTreeData[child].parentNodeIndex = disabledNodeIndex;
        }
        ModelTreeNode disabledNode;
        disabledNode.elementNameIndex = pendingNodeData.elementIndex;
        disabledNode.parentNodeIndex = INDEX_ROOT_DISABLEDNODES;
        disabledNode.childNodeIndices = pendingNodeData.childNodeIndices;
        modelTreeData.push_back(disabledNode);
        modelTreeData[INDEX_ROOT_DISABLEDNODES].childNodeIndices.push_back(disabledNodeIndex);
    }

    modelElementReferenceData.clear();
    modelElementReferenceData.resize(nameList->size());
    for (indextype i = NUM_RESERVED_NODES, n = modelTreeData.size(); i < n; ++i) {
        auto& data = modelTreeData[i];
        Q_ASSERT(data.elementNameIndex >= 0 && data.elementNameIndex < modelElementReferenceData.size());
        auto& refData = modelElementReferenceData[data.elementNameIndex];

        // this is checked in any case
        if (refData.primaryReference_All == 0) {
            refData.primaryReference_All = i;
        }

        if (data.parentNodeIndex == INDEX_ROOT_DISABLEDNODES) {
            // top-level disabled node
            // this does not represent a reference
            continue;
        }

        // these are only for valid references
        if (refData.primaryReference == 0) {
            refData.primaryReference = i;
        }

        data.referenceIndex = refData.numReferences;
        refData.numReferences += 1;
    }

    endResetModel();
    treeWidget->expandAll();
}

QModelIndex HierarchicalElementTreeControllerObject::getFirstIndex() const
{
    QModelIndex index;
    if (!modelTreeData.at(INDEX_ROOT_TOPNODES).childNodeIndices.empty()) {
        index = createIndex(0, 0, modelTreeData.at(INDEX_ROOT_TOPNODES).childNodeIndices.front());
    } else if (!modelTreeData.at(INDEX_ROOT_DISABLEDNODES).childNodeIndices.empty()) {
        // if we reach here then no top nodes are there yet
        index = createIndex(0, 0, modelTreeData.at(INDEX_ROOT_DISABLEDNODES).childNodeIndices.front());
    }
    return index;
}

QModelIndex HierarchicalElementTreeControllerObject::getReferenceForElement(int elementIndex, indextype referenceIndex) const
{
    indextype last = 0;
    for (indextype id = NUM_RESERVED_NODES, n = modelTreeData.size(); id < n; ++id) {
        const auto& data = modelTreeData.at(id);
        if (data.elementNameIndex == elementIndex) {
            if (referenceIndex == 0) {
                return getIndexFromInternalID(id);
            }
            referenceIndex -= 1;
            last = id;
        }
    }
    return getIndexFromInternalID(last);
}

QModelIndex HierarchicalElementTreeControllerObject::getPrimaryIndexForElement(int elementIndex) const
{
    // we just loop over the model tree data to find the first occurrence of the given element
    indextype internalID = modelElementReferenceData.at(elementIndex).primaryReference_All;
    if (internalID == 0) {
        return QModelIndex();
    }
    return getIndexFromInternalID(internalID);
}

indextype HierarchicalElementTreeControllerObject::getNextReferenceAfterRemoving(indextype id, bool skipRefToSameElement) const
{
    Q_ASSERT(id >= NUM_RESERVED_NODES && id < modelTreeData.size());
    const auto& srcRef = modelTreeData.at(id);
    indextype srcElement = srcRef.elementNameIndex;
    // first try to find a reference after the source one
    for (indextype nextID = id + 1, n = modelTreeData.size(); nextID < n; ++nextID) {
        if (skipRefToSameElement) {
            if (modelTreeData.at(nextID).elementNameIndex == srcElement) {
                continue;
            }
        }
        return nextID;
    }
    for (indextype nextID = id - 1; nextID >= NUM_RESERVED_NODES; --nextID) {
        if (skipRefToSameElement) {
            if (modelTreeData.at(nextID).elementNameIndex == srcElement) {
                continue;
            }
        }
        return nextID;
    }
    // okay, no solution
    return 0;
}

QModelIndex HierarchicalElementTreeControllerObject::getIndexFromInternalID(indextype id) const
{
    // we just need to find the row number

    if (id < NUM_RESERVED_NODES || id >= static_cast<decltype(id)>(modelTreeData.size())) {
        return QModelIndex();
    }

    const auto& data = modelTreeData.at(id);

    auto parentID = data.parentNodeIndex;
    int row = -1;
    switch (parentID) {
    case INDEX_ROOT_DISABLEDNODES: {
        const auto& parentData = modelTreeData.at(INDEX_ROOT_DISABLEDNODES);
        indextype idx = parentData.childNodeIndices.indexOf(id);
        Q_ASSERT(idx >= 0 && idx < parentData.childNodeIndices.size());
        row = idx + modelTreeData.at(INDEX_ROOT_TOPNODES).childNodeIndices.size();
    }break;
    case INDEX_ROOT_TOPNODES:
    default: {
        const auto& parentData = modelTreeData.at(parentID);
        row = parentData.childNodeIndices.indexOf(id);
        Q_ASSERT(row >= 0 && row < parentData.childNodeIndices.size());
    }break;
    }
    return createIndex(row, 0, id);
}

QModelIndex HierarchicalElementTreeControllerObject::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) {
        return QModelIndex();
    }
    if (parent.isValid()) {
        // create an index under a parent node
        auto id = parent.internalId();
        Q_ASSERT(id >= 0 && id < static_cast<decltype(id)>(modelTreeData.size()));
        if (id < NUM_RESERVED_NODES) {
            // parent is a reserved node -> no parent in tree view
            return QModelIndex();
        }
        const auto& data = modelTreeData.at(id);
        if (row < 0 || row >= data.childNodeIndices.size()) {
            return QModelIndex();
        }
        auto internalID = data.childNodeIndices.at(row);
        return createIndex(row, column, internalID);
    } else {
        // create an index for top-level node
        const auto& topNodesData = modelTreeData.at(INDEX_ROOT_TOPNODES);
        const auto& disabledNodesData = modelTreeData.at(INDEX_ROOT_DISABLEDNODES);
        auto numTopNodes = topNodesData.childNodeIndices.size();
        auto numDisabledNodes = disabledNodesData.childNodeIndices.size();
        if (row < 0 || row >= (numTopNodes + numDisabledNodes)) {
            return QModelIndex();
        }
        if (row < numTopNodes) {
            auto internalID = topNodesData.childNodeIndices.at(row);
            return createIndex(row, column, internalID);
        }
        auto internalID = disabledNodesData.childNodeIndices.at(row - numTopNodes);
        return createIndex(row, column, internalID);
    }
}

QModelIndex HierarchicalElementTreeControllerObject::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    auto id = index.internalId();
    Q_ASSERT(id >= NUM_RESERVED_NODES && id < static_cast<decltype(id)>(modelTreeData.size()));

    const auto& data = modelTreeData.at(id);
    auto internalID = data.parentNodeIndex;
    if (internalID < NUM_RESERVED_NODES) {
        return QModelIndex();
    }

    const auto& parentData = modelTreeData.at(internalID);
    auto grandParentID = parentData.parentNodeIndex;
    int row = -1;
    switch (grandParentID) {
    case INDEX_ROOT_DISABLEDNODES: {
        const auto& grandParent = modelTreeData.at(INDEX_ROOT_DISABLEDNODES);
        int idx = grandParent.childNodeIndices.indexOf(internalID);
        Q_ASSERT(idx >= 0 && idx < grandParent.childNodeIndices.size());
        row = idx + modelTreeData.at(INDEX_ROOT_TOPNODES).childNodeIndices.size();
    }break;
    case INDEX_ROOT_TOPNODES:
    default: {
        const auto& grandParent = modelTreeData.at(grandParentID);
        row = grandParent.childNodeIndices.indexOf(internalID);
        Q_ASSERT(row >= 0 && row < grandParent.childNodeIndices.size());
    }break;
    }
    return createIndex(row, 0, internalID);
}

int HierarchicalElementTreeControllerObject::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        indextype internalID = parent.internalId();
        Q_ASSERT(internalID >= NUM_RESERVED_NODES);
        return modelTreeData.at(internalID).childNodeIndices.size();
    }
    return modelTreeData.at(INDEX_ROOT_TOPNODES).childNodeIndices.size() + modelTreeData.at(INDEX_ROOT_DISABLEDNODES).childNodeIndices.size();
}

int HierarchicalElementTreeControllerObject::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant HierarchicalElementTreeControllerObject::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    indextype id = index.internalId();
    Q_ASSERT(id >= NUM_RESERVED_NODES);
    const auto& data = modelTreeData.at(id);

    switch (role) {
    default: return QVariant();
    case Qt::ToolTipRole:
    case Qt::DisplayRole: {
        indextype elementIndex = data.elementNameIndex;
        const QString& elementName = nameList->at(elementIndex);
        QString indexAsString = QString::number(elementIndex);
        QString str;
        str.reserve(3 + elementName.length() + indexAsString.length());
        str.append('[');
        str.append(indexAsString);
        str.append(']');
        str.append(' ');
        str.append(elementName);
        return str;
    }break;
    case Qt::ForegroundRole: {
        // display disabled nodes in grey
        // display other nodes in full black color
        if (data.parentNodeIndex == INDEX_ROOT_DISABLEDNODES) {
            return QBrush(QColor(128, 128, 128)); // dark grey for disabled nodes
        }
        return QBrush(QColor(0,0,0)); // black for enabled nodes
    }break;
    }
}

Qt::ItemFlags HierarchicalElementTreeControllerObject::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flag = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;

    if (index.isValid())
        flag |= Qt::ItemIsDragEnabled;

    return flag;
}

Qt::DropActions HierarchicalElementTreeControllerObject::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList HierarchicalElementTreeControllerObject::mimeTypes() const
{
    return QStringList() << PP_MIMETYPE::SimpleParser_RuleNodeReference;
}

namespace {
const QString REF_TYPE_NAME = QStringLiteral("TypeName");
const QString REF_TYPE_HASH = QStringLiteral("TypeHash");
const QString REF_UUID      = QStringLiteral("UUID");
const QString REF_INDEX     = QStringLiteral("Index");
}

QMimeData* HierarchicalElementTreeControllerObject::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }
    indextype index = indexes.front().internalId();
    QJsonObject obj;
    obj.insert(REF_TYPE_NAME,   QJsonValue(controllerTypeName));
    obj.insert(REF_TYPE_HASH,   QJsonValue(QString::number(controllerTypeHash)));
    obj.insert(REF_UUID,        QJsonValue(uuid.toString()));
    obj.insert(REF_INDEX,       QJsonValue(QString::number(index)));
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QMimeData* result = new QMimeData();
    result->setData(PP_MIMETYPE::SimpleParser_RuleNodeReference, data);
    return result;
}

bool HierarchicalElementTreeControllerObject::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(parent);

    if (!data->hasFormat(PP_MIMETYPE::SimpleParser_RuleNodeReference))
        return false;

    if (column > 0)
        return false;

    // try to read data
    QByteArray dataArray = data->data(PP_MIMETYPE::SimpleParser_RuleNodeReference);
    QJsonDocument doc = QJsonDocument::fromJson(dataArray);
    if (doc.isEmpty()) {
        return false;
    }

    // check if the data comes from the same type; cannot copy/move data across different types
    QJsonObject obj = doc.object();
    if (obj.value(REF_TYPE_NAME).toString() != controllerTypeName
     || obj.value(REF_TYPE_HASH).toString() != QString::number(controllerTypeHash)) {
        return false;
    }

    // check if the data comes from the same model; currently we don't support moving across model yet
    if (QUuid::fromString(obj.value(REF_UUID).toString()) != uuid) {
        return false;
    }

    // everything good
    return true;
}

bool HierarchicalElementTreeControllerObject::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    // almost the same code as canDropMimeData() in the first part
    Q_UNUSED(row);

    if (!data->hasFormat(PP_MIMETYPE::SimpleParser_RuleNodeReference))
        return false;

    if (column > 0)
        return false;

    // try to read data
    QByteArray dataArray = data->data(PP_MIMETYPE::SimpleParser_RuleNodeReference);
    QJsonDocument doc = QJsonDocument::fromJson(dataArray);
    if (doc.isEmpty()) {
        return false;
    }

    // check if the data comes from the same type; cannot copy/move data across different types
    QJsonObject obj = doc.object();
    if (obj.value(REF_TYPE_NAME).toString() != controllerTypeName
     || obj.value(REF_TYPE_HASH).toString() != QString::number(controllerTypeHash)) {
        return false;
    }

    // check if the data comes from the same model; currently we don't support moving across model yet
    if (QUuid::fromString(obj.value(REF_UUID).toString()) != uuid) {
        return false;
    }

    // -----------------------------------------------------

    // read the index and do the last check
    indextype id = obj.value(REF_INDEX).toString().toULongLong();
    if (id < NUM_RESERVED_NODES || id > modelTreeData.size()) {
        return false;
    }

    indextype newParentIndex = 0;
    if (parent.isValid()) {
        newParentIndex = parent.internalId();
    }

    bool isAddNewEdge = false;
    bool isRemoveOldEdge = false;

    GraphData::EdgeOperation requestedOp = GraphData::EdgeOperation::Move;
    if (action == Qt::CopyAction) {
        requestedOp = GraphData::EdgeOperation::Copy;
    }

    switch (determineEdgeOperation(id, newParentIndex, requestedOp)) {
    case GraphData::EdgeOperation::NOOP: return false;
    case GraphData::EdgeOperation::Copy: {
        isAddNewEdge = true;
    }break;
    case GraphData::EdgeOperation::Move: {
        isAddNewEdge = true;
        isRemoveOldEdge = true;
    }break;
    }
    Q_ASSERT(isAddNewEdge); // currently this should always be true

    // save the name of elements to set the correct current reference after the change
    QString curElementName = nameList->at(modelTreeData.at(id).elementNameIndex); // make a copy
    QString newParentName = (newParentIndex >= NUM_RESERVED_NODES)? nameList->at(modelTreeData.at(newParentIndex).elementNameIndex): QString();

    if (isRemoveOldEdge) {
        const auto& modelData = modelTreeData.at(id);
        if (modelData.parentNodeIndex == INDEX_ROOT_TOPNODES) {
            graphData->removeTopElementReference(curElementName);
        } else if (modelData.parentNodeIndex >= NUM_RESERVED_NODES) {
            const auto& parentData = modelTreeData.at(modelData.parentNodeIndex);
            graphData->removeEdge(nameList->at(parentData.elementNameIndex), curElementName);
        }
    }
    if (isAddNewEdge) {
        if (newParentIndex >= NUM_RESERVED_NODES) {
            graphData->addEdge(newParentName, curElementName);
        } else {
            graphData->addTopElementReference(curElementName);
        }
    }
    emitTreeUpdated();

    rebuildModelTreeData();

    // go to the reference
    indextype newParentRef = (!newParentName.isEmpty())? modelElementReferenceData.at(nameList->indexOf(newParentName)).primaryReference_All : 0;
    indextype childElementIndex = nameList->indexOf(curElementName); // while this should not be changed, just to be safe
    indextype destRefIndex = modelElementReferenceData.at(childElementIndex).primaryReference_All;
    for (indextype i : modelTreeData.at(newParentRef).childNodeIndices) {
        if (modelTreeData.at(i).elementNameIndex == childElementIndex) {
            destRefIndex = i;
            break;
        }
    }

    gotoReference(destRefIndex);

    // done
    return true;
}

HierarchicalElementTreeControllerObject::GraphData::EdgeOperation HierarchicalElementTreeControllerObject::determineEdgeOperation(indextype draggedReferenceID, indextype newParentID, GraphData::EdgeOperation requestedOp) const
{
    Q_ASSERT(draggedReferenceID >= NUM_RESERVED_NODES && draggedReferenceID < modelTreeData.size());
    const auto& data = modelTreeData.at(draggedReferenceID);

    // first of all, if the new parent is the same as the old parent, then this is a guaranteed no-op
    // (we also exclude adding top-level node reference to disabled node by drag and drop)
    if (data.parentNodeIndex == newParentID || (data.parentNodeIndex < NUM_RESERVED_NODES && newParentID < NUM_RESERVED_NODES))
        return GraphData::EdgeOperation::NOOP;

    if (data.parentNodeIndex >= NUM_RESERVED_NODES && newParentID >= NUM_RESERVED_NODES) {
        if (modelTreeData.at(newParentID).elementNameIndex == data.elementNameIndex) {
            return GraphData::EdgeOperation::NOOP;
        }
    }

    // there is also no ambiguity if there is no valid reference to the element before
    // (i.e., drag a disabled element to another element)
    if (data.parentNodeIndex == INDEX_ROOT_DISABLEDNODES && newParentID >= NUM_RESERVED_NODES) {
        return GraphData::EdgeOperation::Copy;
    }

    // it is also a no-op if we are dragging it to a different parent element but the parent already has a reference to the current element
    const QString& curElementName = nameList->at(data.elementNameIndex);
    QString newParent;
    if (newParentID >= NUM_RESERVED_NODES) {
        const auto& newParentData = modelTreeData.at(newParentID);
        newParent = nameList->at(newParentData.elementNameIndex);
        if (graphData->hasChild(newParent, curElementName)) {
            return GraphData::EdgeOperation::NOOP;
        }
    } else {
        Q_ASSERT(newParentID == INDEX_ROOT_TOPNODES); // the other case (INDEX_ROOT_DISABLEDNODES) should be impossible
        if (graphData->isTopElement(curElementName)) {
            return GraphData::EdgeOperation::NOOP;
        }
    }
    // okay, we need to ask the graph for how to interpret this drag
    // try to get the name of old parent
    QString oldParent;
    if (data.parentNodeIndex >= NUM_RESERVED_NODES) {
        const auto& oldParentData = modelTreeData.at(data.parentNodeIndex);
        oldParent = nameList->at(oldParentData.elementNameIndex);
    } else {
        Q_ASSERT(data.parentNodeIndex == INDEX_ROOT_TOPNODES); // the other case (INDEX_ROOT_DISABLEDNODES) should already be handled
    }

    return graphData->acceptedEdgeOperation(requestedOp, curElementName, oldParent, newParent);
}

