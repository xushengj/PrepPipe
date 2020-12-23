#ifndef HIERARCHICALELEMENTTREECONTROLLER_H
#define HIERARCHICALELEMENTTREECONTROLLER_H

#include <QObject>
#include <QTreeView>
#include <QStackedWidget>
#include <QList>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QAbstractItemModel>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QUuid>

#include <type_traits>
#include <typeinfo>

#include "src/utils/BidirStringList.h"
#include "src/utils/ContiguousIndexVector.h"

/**
 * @brief The HierarchicalElementTreeControllerObject class wraps all signal/slot connection
 *
 * QObject does not work in templated class, so a dummy object is needed to use signal/slots in HierarchicalElementTreeController below.
 * In addition, HierarchicalElementTreeControllerObject serves as the data model of the tree view
 */
class HierarchicalElementTreeControllerObject: public QAbstractItemModel
{
    Q_OBJECT
public:
    template <typename ElementWidget, bool isSortElementByName>
    friend class HierarchicalElementTreeController;

    // a hierarchy data that the user should inherit and implement all methods inside
    // HierarchicalElementTreeControllerObject will present the graph as a DAG in the tree view
    struct GraphData {
    public:
        virtual ~GraphData() = default;

        // read interface
        virtual QStringList getTopElementList() const = 0; // what elements are referred to at the top level
        virtual QStringList getChildElementList(const QString& parentElementName) const = 0; // for each named element, what's its children
        virtual bool isTopElement(const QString& name) const = 0;
        virtual bool hasChild(const QString& parent, const QString& child) const = 0;

        // write interface
        virtual void renameElement(const QString& oldName, const QString& newName) = 0;
        virtual void addEdge(const QString& parent, const QString& newChild) = 0;
        virtual void removeEdge(const QString& parent, const QString& child) = 0;
        virtual void rewriteEdge(const QString& parent, const QString& oldChild, const QString& newChild) {
            removeEdge(parent, oldChild);
            addEdge(parent, newChild);
        }
        virtual void addTopElementReference(const QString& name) = 0;
        virtual void removeTopElementReference(const QString& name) = 0;
        virtual void rewriteTopElementReference(const QString& oldName, const QString& newName) {
            removeTopElementReference(oldName);
            addTopElementReference(newName);
        }
        virtual void removeAllEdgeWith(const QString& name) = 0;
        virtual void notifyNewElementAdded(const QString& name) {Q_UNUSED(name)} // derived class may consider adding the element to top-level reference as such

        // ---------------------------------------------------------------------
        // drag and drop support
        enum class EdgeOperation : int {
            NOOP,   //!< no operation can be performed for this drag and drop
            Copy,   //!< add new edge only
            Move    //!< drop old edge + add new edge
        };

        // helper for the graph logic to intercept problematic edge operations
        // no operation should be performed in this function; the controller object will issue the commands
        // the caller guarantees that:
        // 1. oldParent != newParent
        // 2. the edge from oldParent to current element (name) exists
        // 3. the edge from newParent to current element does not exist
        // (in other words, this function is only called when there is ambiguity)
        // The input requestedOp will not be NOOP
        virtual EdgeOperation acceptedEdgeOperation(EdgeOperation requestedOp, const QString& name, const QString& oldParent, const QString& newParent) const {
            Q_UNUSED(name)
            Q_UNUSED(oldParent)
            Q_UNUSED(newParent)
            return requestedOp;
        }
    };

    // method exposed to the ElementWidget
    const GraphData* getGraphData() const {return graphData;}

// --------------------------------------------------------
// signal / slots exposed to outside
signals:
    void dirty();
    void treeUpdated();

public slots:
    void tryGoToElement(const QString& elementName);
    void tryCreateElement(const QString& elementName);
    void dataChanged();

// --------------------------------------------------------
// those that are not exposed (internal use in controller)

private slots:
    void currentElementChangeHandler();
    void listWidgetContextMenuEventHandler(const QPoint& p);

private:
    // only the controller can instantiate it
    HierarchicalElementTreeControllerObject();
    void init(QTreeView* treeWidgetArg, QStackedWidget* stackedWidgetArg, BidirStringList& nameListArg, GraphData* graphDataArg,
              const char *controllerTypeNameArg, size_t controllerTypeHashArg);

    void setCurrentElementChangeCallback(std::function<void(int)> cb) {
        currentElementChangeCB = cb;
    }
    void setGotoElementCallback(std::function<void(const QString&)> cb) {
        gotoElementCB = cb;
    }
    void setCreateElementCallback(std::function<void(const QString&)> cb) {
        createElementCB = cb;
    }
    void setListWidgetContextMenuEventCallback(std::function<void(const QPoint&)> cb) {
        listWidgetContextMenuEventCB = cb;
    }
    void emitDirty() {
        emit dirty();
    }
    void emitTreeUpdated() {
        emit treeUpdated();
        emit dirty();
    }

private:
    // helper functions for the controller
    QModelIndex getFirstIndex() const; // used when a bunch of data is just loaded
    QModelIndex getIndexFromInternalID(indextype id) const;
    QModelIndex getPrimaryIndexForElement(int elementIndex) const; // used to go to the element after something happens to it (e.g., rename)
    QModelIndex getReferenceForElement(int elementIndex, indextype referenceIndex) const; // get QModelIndex of [referenceIndex]-th reference of element #[elementIndex]
    indextype getNextReferenceAfterRemoving(indextype id, bool skipRefToSameElement) const;// return 0 if no applicable one found

    indextype getElementNameIndex(indextype id) const {
        Q_ASSERT(id>= NUM_RESERVED_NODES && id < modelTreeData.size());
        return modelTreeData.at(id).elementNameIndex;
    }
    indextype getElementNameIndex(const QModelIndex& index) const {
        Q_ASSERT (index.isValid());
        return modelTreeData.at(index.internalId()).elementNameIndex;
    }
    indextype getReferenceIndexInSameElement(indextype id) const {
        Q_ASSERT(id>= NUM_RESERVED_NODES && id < modelTreeData.size());
        return modelTreeData.at(id).referenceIndex;
    }
    indextype getNumValidReferences(int elementIndex) const {
        return modelElementReferenceData.at(elementIndex).numReferences;
    }

    bool isTopLevelReference(indextype id) const {
        Q_ASSERT(id>= NUM_RESERVED_NODES && id < modelTreeData.size());
        return (modelTreeData.at(id).parentNodeIndex < NUM_RESERVED_NODES);
    }
    indextype getParentReference(indextype id) const {
        Q_ASSERT(id>= NUM_RESERVED_NODES && id < modelTreeData.size());
        return modelTreeData.at(id).parentNodeIndex;
    }

    void gotoReference(indextype index) {
        auto* ptr = treeWidget->selectionModel();
        if(ptr != nullptr) {
            ptr->setCurrentIndex(getIndexFromInternalID(index), QItemSelectionModel::Clear);
        }
    }

private:
    void rebuildModelTreeData();
    GraphData::EdgeOperation determineEdgeOperation(indextype draggedReferenceID, indextype newParentID, GraphData::EdgeOperation requestedOp) const; // newParentID is zero for all invalid parent index

protected:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    virtual QModelIndex parent(const QModelIndex &index) const override;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // drag and drop
    // https://doc.qt.io/qt-5/model-view-programming.html#using-drag-and-drop-with-item-views
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    virtual QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList &indexes) const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

private:
    // data specified at init time
    QTreeView* treeWidget = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    BidirStringList* nameList = nullptr;
    GraphData* graphData = nullptr;

    // -------------------------------------------
    // used for drag-drop support
    QUuid uuid; // used for drag/drop handling
    const char* controllerTypeName = nullptr;
    std::size_t controllerTypeHash = 0;
    // -------------------------------------------

    // callbacks for operations
    std::function<void(int)> currentElementChangeCB;
    std::function<void(const QString&)> gotoElementCB;
    std::function<void(const QString&)> createElementCB;
    std::function<void(const QPoint&)> listWidgetContextMenuEventCB;

    // data for the DAG representation
    // each tree element represents a reference of node
    struct ModelTreeNode {
        ContiguousIndexVector<indextype> childNodeIndices;
        indextype elementNameIndex = 0; // index in nameList
        indextype parentNodeIndex = 0;
        indextype referenceIndex = 0; // what's the index of this node/reference for the element
    };
    // two nodes reserved:
    // node 0: pseudo root for top-level nodes
    // node 1: pseudo root for disabled nodes
    // these special nodes will have their parentNodeIndex set to the index of themselves
    static constexpr indextype INDEX_ROOT_TOPNODES = 0;
    static constexpr indextype INDEX_ROOT_DISABLEDNODES = 1;
    static constexpr indextype NUM_RESERVED_NODES = 2;
    QVector<ModelTreeNode> modelTreeData;

    // data for accelerating reference finding
    // one for each element (same size as nameList)
    // make sure it is default-constructed to all zero
    struct ElementReferenceData {
        indextype primaryReference = 0; // what's the (valid) primary reference of this element; 0 if none
        indextype primaryReference_All = 0; // what's the primary reference (including the one from disabled pseudo root node) of this element
        indextype numReferences = 0; // how many references do this element have
    };
    QVector<ElementReferenceData> modelElementReferenceData;
};

template <typename ElementWidget, bool isSortElementByName>
class HierarchicalElementTreeController {
    static_assert (std::is_base_of<QWidget, ElementWidget>::value, "ElementWidget must inherit QWidget!");
    static_assert (std::is_class<typename ElementWidget::StorageData>::value, "Element Widget must have a public struct member called StorageData!");
    // ElementWidget can optionally have a public struct member caled HelperData, which represent the gui helper data that is not present in base object but in GUI object

    // ElementWidget should have the following member functions: (helperData parameter not present if HelperData is void)
    // void setData(const QString& name, const GraphData* graphData, const StorageData& storageData, const HelperData& helperData);
    // void getData(const QString& name, const GraphData* graphData, StorageData&, HelperData&);

    // ElementWidget should have the following signals:
    // void dirty();

    // ElementWidget should have the following slots:
    // void nameUpdated(const QString& newName);
    // void hierarchyChanged(); // any change in direct parent/child relationship shall invoke this slot

public:
    using ElementStorageData = typename ElementWidget::StorageData;
    using ElementHelperData = typename ElementWidget::HelperData;
    using GraphData = typename HierarchicalElementTreeControllerObject::GraphData;

public:
    HierarchicalElementTreeController() = default;
    void init(QTreeView* treeViewArg, QStackedWidget* stackedWidgetArg, GraphData* graphDataArg);

    // get name callback is only used when the name of element is embedded in ElementStorageData
    // if ElementStorageData is always organized in a QHash with its name as key, then this callback is not needed
    void setGetNameCallback(std::function<QString(const ElementStorageData&)> cb) {
        getNameCallback = cb;
    }

    // if the element widget needs additional initialization (e.g. setting callbacks for input field validation),
    // then this callback should be set. If this is not set, then element widgets will be default constructed.
    // the object parameter is the result of getObj() and is to help signal/slot connection
    void setCreateWidgetCallback(std::function<ElementWidget*(HierarchicalElementTreeControllerObject*)> cb) {
        createWidgetCallback = cb;
    }

    // all signal/slots connection goes to the embedded object
    HierarchicalElementTreeControllerObject* getObj() {
        return &obj;
    }
    bool isElementExist(const QString& name) {
        return nameList.contains(name);
    }

    // for all setData() and getData(), the return type is void; the enable_if are for conditionally enable the function with given signature

    // if we have non-void ElementHelperData:
    // if we have the name embedded in ElementStorageData:
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, void>::type setData(const    QVector<ElementStorageData>& data, const    QVector<ElementHelperData>& helperData);
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, void>::type getData(         QVector<ElementStorageData>& data,          QVector<ElementHelperData>& helperData);
    // if we have the name stored out-of-band as the key to a QHash:
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, void>::type setData(const    QHash<QString, ElementStorageData>& data, const QHash<QString, ElementHelperData>& helperData);
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, void>::type getData(         QHash<QString, ElementStorageData>& data,       QHash<QString, ElementHelperData>& helperData);

    // if the ElementHelperData is void:
    // if we have the name embedded in ElementStorageData:
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, void>::type setData(const QVector<ElementStorageData>& data);
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, void>::type getData(      QVector<ElementStorageData>& data);
    // if we have the name stored out-of-band as the key to a QHash:
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, void>::type setData(const QHash<QString, ElementStorageData>& data);
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, void>::type getData(      QHash<QString, ElementStorageData>& data);


private:
    // private functions that show how ElementWidget would be used
    void addElementDuringSetData(const QString& name, ElementWidget* widget) {
        nameList.push_back(name);
        widgetList.push_back(widget);
        QObject::connect(widget, &ElementWidget::dirty, &obj, &HierarchicalElementTreeControllerObject::dirty);
        stackedWidget->addWidget(widget);
    }
    void notifyNameChange(const QString& newName, ElementWidget* widget) {
        widget->nameUpdated(newName);
    }
    void currentElementChangedHandler(int index) {
        if (index == -1) {
            stackedWidget->setCurrentWidget(placeHolderPage);
        } else {
            stackedWidget->setCurrentWidget(widgetList.at(index));
        }
    }
    ElementWidget* createWidget() {
        if (createWidgetCallback) {
            return createWidgetCallback(getObj());
        }
        return new ElementWidget;
    }

    // the two functions below just returns ElementWidget*
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, ElementWidget*>::type
    createWidgetWithData(const QString& name) {
        ElementWidget* w = createWidget();
        w->setData(name, obj.getGraphData(), ElementStorageData());
        return w;
    }
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, ElementWidget*>::type
    createWidgetWithData(const QString& name) {
        ElementWidget* w = createWidget();
        w->setData(name, obj.getGraphData(), ElementStorageData(), ElementHelperData());
        return w;
    }
    // same
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, ElementWidget*>::type
    createWidgetWithDataFromSource(const QString& name, const QString& srcName, ElementWidget* src) {
        ElementStorageData storage;
        src->getData(srcName, obj.getGraphData(), storage);
        ElementWidget* w = createWidget();
        w->setData(name, obj.getGraphData(), storage);
        return w;
    }
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, ElementWidget*>::type
    createWidgetWithDataFromSource(const QString& name, const QString& srcName, ElementWidget* src) {
        ElementStorageData storage;
        ElementHelperData helper;
        src->getData(srcName, obj.getGraphData(), storage, helper);
        ElementWidget* w = createWidget();
        w->setData(name, obj.getGraphData(), storage, helper);
        return w;
    }

    // other (ElementWidget type agnostic) helper functions
    void clearData();
    void finalizeSetData();
    void dataUpdated();
    void tryGoToElement(const QString& element);
    void listWidgetContextMenuEventHandler(const QPoint& pos);
    QString getNameEditDialog(const QString& oldName, bool isNewInsteadofRename);
    void handleRename(indextype id, const QString& newName);
    void handleDropReference(indextype id, bool dropAllInsteadOfOne);
    void handleDelete(indextype id);
    void handleNew(const QString& name);
    void handleCopyAs(const QString& name, const QString& srcName, const QModelIndex& srcIndex);
    void handleAddElement(const QString& name, ElementWidget* w);

    QVector<std::pair<QString, ElementWidget*>> getCombinedPairVec(const QStringList &actualNameList, int reserveSize);

    void gotoReference(const QModelIndex& index) {
        auto* ptr = treeView->selectionModel();
        if(ptr != nullptr) {
            ptr->setCurrentIndex(index, QItemSelectionModel::Clear);
        }
    }
    QModelIndex getCurrentReference() {
        auto* ptr = treeView->selectionModel();
        Q_ASSERT(ptr != nullptr);
        return ptr->currentIndex();
    }

private:
    HierarchicalElementTreeControllerObject obj;
    QTreeView* treeView = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    std::function<QString(const ElementStorageData&)> getNameCallback;
    std::function<ElementWidget*(HierarchicalElementTreeControllerObject*)> createWidgetCallback;

    QWidget* dialogParent = nullptr;
    QWidget* placeHolderPage = nullptr;

    BidirStringList nameList;
    QList<ElementWidget*> widgetList;
};

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::init(QTreeView* treeViewArg, QStackedWidget* stackedWidgetArg, GraphData* graphDataArg)
{
    Q_ASSERT(treeViewArg != nullptr && stackedWidgetArg != nullptr && graphDataArg != nullptr);

    treeView = treeViewArg;
    stackedWidget = stackedWidgetArg;

    dialogParent = stackedWidgetArg;

    // there must be a place holder page / widget in the stack widget and nothing else should be there
    Q_ASSERT(stackedWidget->count() == 1);
    placeHolderPage = stackedWidget->currentWidget();

    obj.init(treeViewArg, stackedWidgetArg, nameList, graphDataArg, typeid (*this).name(), typeid (*this).hash_code());

    obj.setCurrentElementChangeCallback(std::bind(&HierarchicalElementTreeController<ElementWidget, isSortElementByName>::currentElementChangedHandler, this, std::placeholders::_1));
    obj.setGotoElementCallback(std::bind(&HierarchicalElementTreeController<ElementWidget, isSortElementByName>::tryGoToElement, this, std::placeholders::_1));
    obj.setCreateElementCallback(std::bind(&HierarchicalElementTreeController<ElementWidget, isSortElementByName>::handleNew, this, std::placeholders::_1));
    obj.setListWidgetContextMenuEventCallback(std::bind(&HierarchicalElementTreeController<ElementWidget, isSortElementByName>::listWidgetContextMenuEventHandler, this, std::placeholders::_1));
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::clearData()
{
    if (nameList.isEmpty())
        return;

    stackedWidget->setCurrentWidget(placeHolderPage);

    for (QWidget* editor : widgetList) {
        stackedWidget->removeWidget(editor);
        editor->deleteLater();
    }

    nameList.clear();
    widgetList.clear();
    obj.dataChanged();
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::finalizeSetData()
{
    obj.dataChanged();
    if (!widgetList.isEmpty()) {
        gotoReference(obj.getFirstIndex());
    }
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::tryGoToElement(const QString& element)
{
    // this would also emit signal that cause stackedWidget to be updated
    int index = nameList.indexOf(element);
    if (index != -1) {
        currentElementChangedHandler(index);
    }
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::dataUpdated()
{
    obj.dataChanged();
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::setData(
        const QVector<ElementStorageData>& data,
        const QVector<ElementHelperData> &helperData
){
    Q_ASSERT(getNameCallback);
    int numElements = data.size();
    Q_ASSERT(numElements == helperData.size());
    clearData();
    QVector<std::pair<QString, int>> indexVec;
    if (isSortElementByName) {
        for (int i = 0; i < numElements; ++i) {
            const ElementStorageData& storage = data.at(i);
            QString name = getNameCallback(storage);
            indexVec.push_back(std::make_pair(name, i));
        }
        NameSorting::sortNameListWithData(indexVec);
    }
    for (int i = 0; i < numElements; ++i) {
        int indexToUse = (isSortElementByName? indexVec.at(i).second : i);
        const ElementStorageData& storage = data.at(indexToUse);
        const ElementHelperData& helper = helperData.at(indexToUse);
        QString name = (isSortElementByName? indexVec.at(i).first : getNameCallback(storage));
        ElementWidget* widget = createWidget();
        widget->setData(name, obj.getGraphData(), storage, helper);
        addElementDuringSetData(name, widget);
    }
    finalizeSetData();
}


template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::getData(
        QVector<ElementStorageData>& data,
        QVector<ElementHelperData>& helperData)
{
    int numElement = nameList.size();
    Q_ASSERT(numElement == widgetList.size());
    data.resize(numElement);
    helperData.resize(numElement);
    for (int i = 0; i < numElement; ++i) {
        ElementWidget* widget = widgetList.at(i);
        widget->getData(nameList.at(i), obj.getGraphData(), data[i], helperData[i]);
    }
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::setData(
        const QHash<QString, ElementStorageData>& data,
        const QHash<QString, ElementHelperData>& helperData)
{
    int numElements = data.size();
    Q_ASSERT(numElements == helperData.size());
    clearData();
    if (isSortElementByName) {
        QStringList keyList = data.uniqueKeys();
        Q_ASSERT(keyList.size() == numElements);
        NameSorting::sortNameList(keyList);
        for (const QString& name : keyList) {
            auto iterData = data.find(name);
            Q_ASSERT(iterData != data.end());
            auto iterHelperData = helperData.find(name);
            Q_ASSERT(iterHelperData != helperData.end());
            const ElementStorageData& storage = iterData.value();
            const ElementHelperData& helper = iterHelperData.value();
            ElementWidget* widget = createWidget();
            widget->setData(name, obj.getGraphData(), storage, helper);
            addElementDuringSetData(name, widget);
        }
    } else {
        for (auto iterData = data.begin(), iterDataEnd = data.end(); iterData != iterDataEnd; ++iterData) {
            const QString& name = iterData.key();
            const ElementStorageData& storage = iterData.value();
            auto iterHelperData = helperData.find(name);
            Q_ASSERT(iterHelperData != helperData.end());
            const ElementHelperData& helper = iterHelperData.value();
            ElementWidget* widget = createWidget();
            widget->setData(name, obj.getGraphData(), storage, helper);
            addElementDuringSetData(name, widget);
        }
    }
    finalizeSetData();
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::getData(
        QHash<QString, ElementStorageData>& data,
        QHash<QString, ElementHelperData>& helperData)
{
    data.clear();
    helperData.clear();
    for (int i = 0, numElements = nameList.size(); i < numElements; ++i) {
        const QString& name = nameList.at(i);
        ElementWidget* widget = widgetList.at(i);
        ElementStorageData storage;
        ElementHelperData helper;
        widget->getData(name, obj.getGraphData(), storage, helper);
        data.insert(name, storage);
        helperData.insert(name, helper);
    }
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::setData(const QVector<ElementStorageData>& data)
{
    Q_ASSERT(getNameCallback);
    int numElements = data.size();
    clearData();
    QVector<std::pair<QString, int>> indexVec;
    if (isSortElementByName) {
        for (int i = 0; i < numElements; ++i) {
            const ElementStorageData& storage = data.at(i);
            QString name = getNameCallback(storage);
            indexVec.push_back(std::make_pair(name, i));
        }
        NameSorting::sortNameListWithData(indexVec);
    }
    for (int i = 0; i < numElements; ++i) {
        int indexToUse = (isSortElementByName? indexVec.at(i).second : i);
        const ElementStorageData& storage = data.at(indexToUse);
        QString name = (isSortElementByName? indexVec.at(i).first : getNameCallback(storage));
        ElementWidget* widget = createWidget();
        widget->setData(name, obj.getGraphData(), storage);
        addElementDuringSetData(name, widget);
    }
    finalizeSetData();
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::getData(QVector<ElementStorageData> &data)
{
    int numElement = nameList.size();
    Q_ASSERT(numElement == widgetList.size());
    data.resize(numElement);
    for (int i = 0; i < numElement; ++i) {
        ElementWidget* widget = widgetList.at(i);
        widget->getData(nameList.at(i), obj.getGraphData(), data[i]);
    }
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::setData(const QHash<QString, ElementStorageData>& data)
{
    int numElements = data.size();
    clearData();
    if (isSortElementByName) {
        QStringList keyList = data.uniqueKeys();
        Q_ASSERT(keyList.size() == numElements);
        NameSorting::sortNameList(keyList);
        for (const QString& name : keyList) {
            auto iterData = data.find(name);
            Q_ASSERT(iterData != data.end());
            const ElementStorageData& storage = iterData.value();
            ElementWidget* widget = createWidget();
            widget->setData(name, obj.getGraphData(), storage);
            addElementDuringSetData(name, widget);
        }
    } else {
        for (auto iterData = data.begin(), iterDataEnd = data.end(); iterData != iterDataEnd; ++iterData) {
            const QString& name = iterData.key();
            const ElementStorageData& storage = iterData.value();
            ElementWidget* widget = createWidget();
            widget->setData(name, obj.getGraphData(), storage);
            addElementDuringSetData(name, widget);
        }
    }
    finalizeSetData();
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::getData(QHash<QString, ElementStorageData> &data)
{
    data.clear();
    for (int i = 0, numElements = nameList.size(); i < numElements; ++i) {
        const QString& name = nameList.at(i);
        ElementWidget* widget = widgetList.at(i);
        ElementStorageData storage;
        widget->getData(name, obj.getGraphData(), storage);
        data.insert(name, storage);
    }
}

template <typename ElementWidget, bool isSortElementByName>
QVector<std::pair<QString, ElementWidget*>>
HierarchicalElementTreeController<ElementWidget, isSortElementByName>::getCombinedPairVec(const QStringList& actualNameList, int reserveSize)
{
    QVector<std::pair<QString, ElementWidget*>> combinedPair;
    int numElements = actualNameList.size();
    Q_ASSERT(numElements == widgetList.size());
    Q_ASSERT(numElements <= reserveSize);
    combinedPair.reserve(reserveSize);
    for (int i = 0; i < numElements; ++i) {
        QString name = actualNameList.at(i);
        ElementWidget* curWidget = widgetList.at(i);
        combinedPair.push_back(std::make_pair(name, curWidget));
    }
    return combinedPair;
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::handleRename(indextype id, const QString& newName)
{
    int elementIndex = obj.getElementNameIndex(id);
    indextype refIdx = obj.getReferenceIndexInSameElement(id);
    Q_ASSERT(elementIndex >= 0 && elementIndex < nameList.size());
    QStringList curList = nameList.getList();
    QString oldName = curList[elementIndex];
    curList[elementIndex] = newName;
    // we keep a pointer here and push the notification after all the invariants are recovered
    ElementWidget* w = widgetList.at(elementIndex);
    int numElements = nameList.size();
    QVector<std::pair<QString, ElementWidget*>> combinedPair = getCombinedPairVec(curList, numElements);
    NameSorting::sortNameListWithData(combinedPair);
    int curIndex = -1;
    for (int i = 0; i < numElements; ++i) {
        auto& p = combinedPair.at(i);
        curList[i] = p.first;
        widgetList[i] = p.second;
        if (p.second == w) {
            curIndex = i;
        }
    }
    nameList = curList;
    obj.graphData->renameElement(oldName, newName);
    dataUpdated();
    notifyNameChange(newName, w);
    Q_ASSERT(curIndex >= 0);
    gotoReference(obj.getReferenceForElement(curIndex, refIdx));
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::handleDelete(indextype id)
{
    int elementIndex = obj.getElementNameIndex(id);
    Q_ASSERT(elementIndex >= 0 && elementIndex < widgetList.size());
    ElementWidget* currentWidget = widgetList.at(elementIndex);
    indextype nextRef = getCurrentReference().internalId();
    if (stackedWidget->currentWidget() == currentWidget) {
        if (widgetList.size() > 1) {
            nextRef = obj.getNextReferenceAfterRemoving(id, true);
        } else {
            nextRef = 0;
        }
    }
    int nextElementIndex = -1; // this index can be stale after the change to nameList, but the next two variables do not
    QString nextElementName;
    indextype nextRefIdx = 0;
    if (nextRef > 0) {
        nextElementIndex = obj.getElementNameIndex(nextRef);
        nextElementName = nameList.at(nextElementIndex);
        nextRefIdx = obj.getReferenceIndexInSameElement(nextRef);
    }
    stackedWidget->removeWidget(currentWidget);
    currentWidget->deleteLater();
    widgetList.removeAt(elementIndex);
    obj.graphData->removeAllEdgeWith(nameList.at(elementIndex));
    nameList.removeAt(elementIndex);
    dataUpdated();
    QModelIndex nextIndex = obj.getFirstIndex();
    if (nextElementIndex >= 0) {
        nextElementIndex = nameList.indexOf(nextElementName);
        nextIndex = obj.getReferenceForElement(nextElementIndex, nextRefIdx);
    }
    gotoReference(nextIndex);
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::handleDropReference(indextype id, bool dropAllInsteadOfOne)
{
    indextype nextRef = getCurrentReference().internalId();
    if (nextRef == id) {
        nextRef = obj.getNextReferenceAfterRemoving(id, dropAllInsteadOfOne);
    }
    int nextElementIndex = -1;
    indextype nextRefIdx = 0;
    if (nextRef > 0) {
        nextElementIndex = obj.getElementNameIndex(nextRef);
        nextRefIdx = obj.getReferenceIndexInSameElement(nextRef);
    }

    int curElement = obj.getElementNameIndex(id);
    const QString& name = nameList.at(curElement);
    if (dropAllInsteadOfOne) {
        obj.graphData->removeAllEdgeWith(name);
    } else {
        if (obj.isTopLevelReference(id)) {
            obj.graphData->removeTopElementReference(name);
        } else {
            indextype parentReference = obj.getParentReference(id);
            int parentElement = obj.getElementNameIndex(parentReference);
            obj.graphData->removeEdge(nameList.at(parentElement), name);
        }
    }

    dataUpdated();
    QModelIndex nextIndex = obj.getFirstIndex();
    if (nextElementIndex >= 0) {
        nextIndex = obj.getReferenceForElement(nextElementIndex, nextRefIdx);
    }
    gotoReference(nextIndex);
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::handleAddElement(const QString& name, ElementWidget* newWidget)
{
    QVector<std::pair<QString, ElementWidget*>> combinedPairVec = getCombinedPairVec(nameList.getList(), nameList.size() + 1);
    combinedPairVec.push_back(std::make_pair(name, newWidget));
    NameSorting::sortNameListWithData(combinedPairVec);
    int numElements = combinedPairVec.size();
    nameList.clear();
    nameList.reserve(numElements);
    widgetList.clear();
    widgetList.reserve(numElements);
    int curIndex = -1;
    for (int i = 0; i < numElements; ++i) {
        const auto& p = combinedPairVec.at(i);
        nameList.push_back(p.first);
        widgetList.push_back(p.second);
        if (p.second == newWidget) {
            curIndex = i;
        }
    }
    stackedWidget->addWidget(newWidget);
    obj.graphData->notifyNewElementAdded(name);
    dataUpdated();
    Q_ASSERT(curIndex >= 0);
    gotoReference(obj.getPrimaryIndexForElement(curIndex));
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::handleNew(const QString& name)
{
    // because beside the call from right click menu, this function is also called by incoming signal connection,
    // we need to check if the name is already used, and just early exit if it is the case.
    if (isElementExist(name))
        return;

    ElementWidget* newWidget = createWidgetWithData(name);
    handleAddElement(name, newWidget);
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::handleCopyAs(const QString& name, const QString& srcName, const QModelIndex& srcIndex)
{
    // we need to:
    // 1. update the source reference to the old element to use the new element instead
    // 2. create the new element
    // make sure the edge to the new element is added before creating the new element so that no top-level reference is added by mistake
    int elementIndex = obj.getElementNameIndex(srcIndex);
    QModelIndex parentIndex = obj.parent(srcIndex);
    if (parentIndex.isValid()) {
        // need to update the reference to point to the new one
        int parentElementIndex = obj.getElementNameIndex(parentIndex);
        QString parentName = nameList.at(parentElementIndex);
        obj.graphData->rewriteEdge(parentName, srcName, name);
    } else if (obj.graphData->isTopElement(name)) {
        obj.graphData->rewriteTopElementReference(srcName, name);
    }
    ElementWidget* src = widgetList.at(elementIndex);
    ElementWidget* newWidget = createWidgetWithDataFromSource(name, srcName, src);
    handleAddElement(name, newWidget);
}

template <typename ElementWidget, bool isSortElementByName>
void HierarchicalElementTreeController<ElementWidget, isSortElementByName>::listWidgetContextMenuEventHandler(const QPoint& pos)
{
    QModelIndex index = treeView->indexAt(pos);
    QMenu menu(treeView);
    QString oldName;
    if (index.isValid()) {
        int elementIndex = obj.getElementNameIndex(index);
        Q_ASSERT(elementIndex >= 0 && elementIndex < nameList.size());
        oldName = nameList.at(elementIndex);

        QAction* renameAction = new QAction(HierarchicalElementTreeControllerObject::tr("Rename"));
        QObject::connect(renameAction, &QAction::triggered, getObj(), [=](){
            QString newName = getNameEditDialog(oldName, false);
            if (newName.isEmpty() || newName == oldName)
                return;
            if (isElementExist(newName)) {
                QMessageBox::warning(dialogParent,
                                     HierarchicalElementTreeControllerObject::tr("Rename failed"),
                                     HierarchicalElementTreeControllerObject::tr("There is already an entry named \"%1\".").arg(newName));
                tryGoToElement(newName);
                return;
            }
            handleRename(index.internalId(), newName);
        });
        menu.addAction(renameAction);

        QAction* copyAsAction = new QAction(HierarchicalElementTreeControllerObject::tr("Copy/Fork As"));
        QObject::connect(copyAsAction, &QAction::triggered, getObj(), [=](){
            QString newName = getNameEditDialog(oldName, true);
            if (newName.isEmpty() || newName == oldName)
                return;
            if (isElementExist(newName)) {
                QMessageBox::warning(dialogParent,
                                     HierarchicalElementTreeControllerObject::tr("Copy/Fork failed"),
                                     HierarchicalElementTreeControllerObject::tr("There is already an entry named \"%1\".").arg(newName));
                tryGoToElement(newName);
                return;
            }
            handleCopyAs(newName, oldName, index);
        });
        menu.addAction(copyAsAction);

        indextype numReferences = obj.getNumValidReferences(elementIndex);
        if (numReferences > 0) {
            QAction* removeAction = new QAction(HierarchicalElementTreeControllerObject::tr("Remove"));
            QObject::connect(removeAction, &QAction::triggered, getObj(), [=](){
                if (QMessageBox::question(dialogParent,
                                          HierarchicalElementTreeControllerObject::tr("Remove confirmation"),
                                          HierarchicalElementTreeControllerObject::tr("Are you sure you want to remove all %1 reference(s) to \"%2\"?")
                                            .arg(QString::number(numReferences), oldName),
                                          QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes) {
                    handleDropReference(index.internalId(), true);
                }
            });
            menu.addAction(removeAction);
            if (numReferences > 1) {
                QAction* removeSingleAction = new QAction(HierarchicalElementTreeControllerObject::tr("Remove This"));
                QObject::connect(removeSingleAction, &QAction::triggered, getObj(), [=](){
                    if (QMessageBox::question(dialogParent,
                                              HierarchicalElementTreeControllerObject::tr("Remove confirmation"),
                                              HierarchicalElementTreeControllerObject::tr("Are you sure you want to remove this reference to \"%1\"? Other references are not affected.")
                                                .arg(oldName),
                                              QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes) {
                        handleDropReference(index.internalId(), false);
                    }
                });
                menu.addAction(removeSingleAction);
            }
        } else {
            // this node is disabled
            QAction* deleteAction = new QAction(HierarchicalElementTreeControllerObject::tr("Delete"));
            QObject::connect(deleteAction, &QAction::triggered, getObj(), [=](){
                if (QMessageBox::question(dialogParent,
                                          HierarchicalElementTreeControllerObject::tr("Delete confirmation"),
                                          HierarchicalElementTreeControllerObject::tr("Are you sure you want to delete \"%1\"?").arg(oldName),
                                          QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes) {
                    handleDelete(index.internalId());
                }
            });
            menu.addAction(deleteAction);
        }
    }
    QAction* newAction = new QAction(HierarchicalElementTreeControllerObject::tr("New"));
    QObject::connect(newAction, &QAction::triggered, getObj(), [=](){
        QString newName = getNameEditDialog(oldName, true);
        if (newName.isEmpty() || newName == oldName)
            return;
        // if the name is already in use, pop up a dialog
        if (isElementExist(newName)) {
            QMessageBox::warning(dialogParent,
                                 HierarchicalElementTreeControllerObject::tr("Creation failed"),
                                 HierarchicalElementTreeControllerObject::tr("There is already an entry named \"%1\".").arg(newName));
            tryGoToElement(newName);
            return;
        }
        handleNew(newName);
    });
    menu.addAction(newAction);
    menu.exec(treeView->mapToGlobal(pos));
}

template <typename ElementWidget, bool isSortElementByName>
QString HierarchicalElementTreeController<ElementWidget, isSortElementByName>::getNameEditDialog(const QString& oldName, bool isNewInsteadofRename)
{
    QString title;
    QString prompt;
    if (isNewInsteadofRename) {
        title = HierarchicalElementTreeControllerObject::tr("New");
        prompt = HierarchicalElementTreeControllerObject::tr("Please input the name:");
    } else {
        title = HierarchicalElementTreeControllerObject::tr("Rename");
        prompt = HierarchicalElementTreeControllerObject::tr("Please input the new name for \"%1\":").arg(oldName);
    }
    bool ok = false;
    QString result = QInputDialog::getText(dialogParent, title, prompt, QLineEdit::Normal, oldName, &ok);
    if (ok) {
        return result;
    }
    return QString();
}

#endif // HIERARCHICALELEMENTTREECONTROLLER_H
