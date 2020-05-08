#ifndef NAMEDELEMENTLISTCONTROLLER_H
#define NAMEDELEMENTLISTCONTROLLER_H

#include <QObject>
#include <QListWidget>
#include <QStackedWidget>
#include <QList>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QStringListModel>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>

#include <type_traits>

#include "src/utils/NameSorting.h"

/**
 * @brief The NamedElementListControllerObject class wraps all signal/slot connection
 *
 * QObject does not work in templated class, so a dummy object is needed to use signal/slots in NamedElementListController below.
 */
class NamedElementListControllerObject: public QObject
{
    Q_OBJECT
public:
    template <typename ElementWidget, bool isSortElementByName>
    friend class NamedElementListController;

// --------------------------------------------------------
// signal / slots exposed to outside
signals:
    void dirty();
    void listUpdated(const QStringList& list);

public slots:
    void tryGoToElement(const QString& elementName);
    void tryCreateElement(const QString& elementName);

// --------------------------------------------------------
// those that are not exposed (internal use in controller)

private slots:
    void currentElementChangeHandler(int index);
    void listWidgetContextMenuEventHandler(const QPoint& p);
private:
    // only the controller can instantiate it
    NamedElementListControllerObject();
    void init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg);
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
    void emitListUpdated(const QStringList& list) {
        emit listUpdated(list);
        emit dirty();
    }

private:
    QListWidget* listWidget = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    std::function<void(int)> currentElementChangeCB;
    std::function<void(const QString&)> gotoElementCB;
    std::function<void(const QString&)> createElementCB;
    std::function<void(const QPoint&)> listWidgetContextMenuEventCB;
};

template <typename ElementWidget, bool isSortElementByName>
class NamedElementListController {
    static_assert (std::is_base_of<QWidget, ElementWidget>::value, "ElementWidget must inherit QWidget!");
    static_assert (std::is_class<typename ElementWidget::StorageData>::value, "Element Widget must have a public struct member called StorageData!");
    // ElementWidget can optionally have a public struct member caled HelperData, which represent the gui helper data that is not present in base object but in GUI object

    // ElementWidget should have the following member functions: (helperData parameter not present if HelperData is void)
    // void setData(const QString& name, const StorageData& storageData, const HelperData& helperData);
    // void getData(const QString& name, StorageData&, HelperData&);

    // ElementWidget should have the following signals:
    // void dirty();

    // ElementWidget should have the following slots:
    // void nameUpdated(const QString& newName);

public:
    using ElementStorageData = typename ElementWidget::StorageData;
    using ElementHelperData = typename ElementWidget::HelperData;

public:
    NamedElementListController() = default;
    void init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg);

    // get name callback is only used when the name of element is embedded in ElementStorageData
    // if ElementStorageData is always organized in a QHash with its name as key, then this callback is not needed
    void setGetNameCallback(std::function<QString(const ElementStorageData&)> cb) {
        getNameCallback = cb;
    }

    // if the element widget needs additional initialization (e.g. setting callbacks for input field validation),
    // then this callback should be set. If this is not set, then element widgets will be default constructed.
    // the object parameter is the result of getObj() and is to help signal/slot connection
    void setCreateWidgetCallback(std::function<ElementWidget*(NamedElementListControllerObject*)> cb) {
        createWidgetCallback = cb;
    }

    // all signal/slots connection goes to the embedded object
    NamedElementListControllerObject* getObj() {
        return &obj;
    }
    QStringListModel* getNameListModel() {
        return nameListModel;
    }
    bool isElementExist(const QString& name) {
        return (nameSearchMap.find(name) != nameSearchMap.end());
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
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, void>::type getData(const QVector<ElementStorageData>& data);
    // if we have the name stored out-of-band as the key to a QHash:
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, void>::type setData(const QHash<QString, ElementStorageData>& data);
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, void>::type getData(const QHash<QString, ElementStorageData>& data);


private:
    // private functions that show how ElementWidget would be used
    void addElementDuringSetData(const QString& name, ElementWidget* widget) {
        int index = nameList.size();
        nameList.push_back(name);
        widgetList.push_back(widget);
        nameSearchMap.insert(name, index);
        QObject::connect(widget, &ElementWidget::dirty, &obj, &NamedElementListControllerObject::dirty);
        listWidget->addItem(name);
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
        w->setData(name, ElementStorageData());
        return w;
    }
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, ElementWidget*>::type
    createWidgetWithData(const QString& name) {
        ElementWidget* w = createWidget();
        w->setData(name, ElementStorageData(), ElementHelperData());
        return w;
    }
    // same
    template <typename Helper = ElementHelperData> typename std::enable_if<std::is_void<Helper>::value, ElementWidget*>::type
    createWidgetWithDataFromSource(const QString& name, const QString& srcName, ElementWidget* src) {
        ElementStorageData storage;
        src->getData(srcName, storage);
        ElementWidget* w = createWidget();
        w->setData(name, storage);
        return w;
    }
    template <typename Helper = ElementHelperData> typename std::enable_if<!std::is_void<Helper>::value, ElementWidget*>::type
    createWidgetWithDataFromSource(const QString& name, const QString& srcName, ElementWidget* src) {
        ElementStorageData storage;
        ElementHelperData helper;
        src->getData(srcName, storage, helper);
        ElementWidget* w = createWidget();
        w->setData(name, storage, helper);
        return w;
    }

    // other (ElementWidget type agnostic) helper functions
    void clearData();
    void finalizeSetData();
    void nameListUpdated();
    void tryGoToElement(const QString& element);
    void listWidgetContextMenuEventHandler(const QPoint& pos);
    QString getNameEditDialog(const QString& oldName, bool isNewInsteadofRename);
    void handleRename(int index, const QString& newName);
    void handleDelete(int index);
    void handleNew(const QString& name);
    void handleCopyAs(const QString& name, const QString& srcName, ElementWidget* src);
    void handleAddElement(const QString& name, ElementWidget* w);

    template<typename... Types>
    static QString tr(Types... arg) {
        return NamedElementListControllerObject::tr(arg...);
    }

    QVector<std::pair<QString, ElementWidget*>> getCombinedPairVec(int reserveSize);

private:
    NamedElementListControllerObject obj;
    QListWidget* listWidget = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    std::function<QString(const ElementStorageData&)> getNameCallback;
    std::function<ElementWidget*(NamedElementListControllerObject*)> createWidgetCallback;

    QWidget* dialogParent = nullptr;
    QWidget* placeHolderPage = nullptr;
    QStringListModel* nameListModel = nullptr;

    QHash<QString, int> nameSearchMap; // Name -> index in nameList
    QStringList nameList;
    QList<ElementWidget*> widgetList;
};

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg)
{
    listWidget = listWidgetArg;
    stackedWidget = stackedWidgetArg;

    dialogParent = stackedWidgetArg;
    nameListModel = new QStringListModel(&obj);

    // there must be a place holder page / widget in the stack widget and nothing else should be there
    Q_ASSERT(stackedWidget->count() == 1);
    placeHolderPage = stackedWidget->currentWidget();

    obj.init(listWidgetArg, stackedWidgetArg);

    obj.setCurrentElementChangeCallback(std::bind(&NamedElementListController<ElementWidget, isSortElementByName>::currentElementChangedHandler, this, std::placeholders::_1));
    obj.setGotoElementCallback(std::bind(&NamedElementListController<ElementWidget, isSortElementByName>::tryGoToElement, this, std::placeholders::_1));
    obj.setCreateElementCallback(std::bind(&NamedElementListController<ElementWidget, isSortElementByName>::handleNew, this, std::placeholders::_1));
    obj.setListWidgetContextMenuEventCallback(std::bind(&NamedElementListController<ElementWidget, isSortElementByName>::listWidgetContextMenuEventHandler, this, std::placeholders::_1));
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::clearData()
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
    nameSearchMap.clear();
    nameListModel->setStringList(nameList);
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::finalizeSetData()
{
    nameListModel->setStringList(nameList);
    if (!widgetList.isEmpty()) {
        stackedWidget->setCurrentWidget(widgetList.front());
    }
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::tryGoToElement(const QString& element)
{
    int index = nameSearchMap.value(element, -1);

    // this would also emit signal that cause stackedWidget to be updated
    listWidget->setCurrentRow(index);
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::nameListUpdated()
{
    listWidget->clear();
    listWidget->addItems(nameList);

    nameSearchMap.clear();
    for (int i = 0, n = nameList.size(); i < n; ++i) {
        nameSearchMap.insert(nameList.at(i), i);
    }

    obj.emitListUpdated(nameList);
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::setData(
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
        widget->setData(name, storage, helper);
        addElementDuringSetData(name, widget);
    }
    finalizeSetData();
}


template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::getData(
        QVector<ElementStorageData>& data,
        QVector<ElementHelperData>& helperData)
{
    int numElement = nameList.size();
    Q_ASSERT(numElement == widgetList.size());
    data.resize(numElement);
    helperData.resize(numElement);
    for (int i = 0; i < numElement; ++i) {
        ElementWidget* widget = widgetList.at(i);
        widget->getData(nameList.at(i), data[i], helperData[i]);
    }
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::setData(
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
            widget->setData(name, storage, helper);
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
            widget->setData(name, storage, helper);
            addElementDuringSetData(name, widget);
        }
    }
    finalizeSetData();
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<!std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::getData(
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
        widget->getData(name, storage, helper);
        data.insert(name, storage);
        helperData.insert(name, helper);
    }
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::setData(const QVector<ElementStorageData>& data)
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
        widget->setData(name, storage);
        addElementDuringSetData(name, widget);
    }
    finalizeSetData();
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::getData(const QVector<ElementStorageData>& data)
{
    int numElement = nameList.size();
    Q_ASSERT(numElement == widgetList.size());
    data.resize(numElement);
    for (int i = 0; i < numElement; ++i) {
        ElementWidget* widget = widgetList.at(i);
        widget->getData(nameList.at(i), data[i]);
    }
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::setData(const QHash<QString, ElementStorageData>& data)
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
            widget->setData(name, storage);
            addElementDuringSetData(name, widget);
        }
    } else {
        for (auto iterData = data.begin(), iterDataEnd = data.end(); iterData != iterDataEnd; ++iterData) {
            const QString& name = iterData.key();
            const ElementStorageData& storage = iterData.value();
            ElementWidget* widget = createWidget();
            widget->setData(name, storage);
            addElementDuringSetData(name, widget);
        }
    }
    finalizeSetData();
}

template <typename ElementWidget, bool isSortElementByName> template <typename Helper>
typename std::enable_if<std::is_void<Helper>::value, void>::type
NamedElementListController<ElementWidget, isSortElementByName>::getData(const QHash<QString, ElementStorageData>& data)
{
    data.clear();
    for (int i = 0, numElements = nameList.size(); i < numElements; ++i) {
        const QString& name = nameList.at(i);
        ElementWidget* widget = widgetList.at(i);
        ElementStorageData storage;
        widget->getData(name, storage);
        data.insert(name, storage);
    }
}

template <typename ElementWidget, bool isSortElementByName>
QVector<std::pair<QString, ElementWidget*>>
NamedElementListController<ElementWidget, isSortElementByName>::getCombinedPairVec(int reserveSize)
{
    QVector<std::pair<QString, ElementWidget*>> combinedPair;
    int numElements = nameList.size();
    Q_ASSERT(numElements == widgetList.size());
    Q_ASSERT(numElements <= reserveSize);
    combinedPair.reserve(reserveSize);
    for (int i = 0; i < numElements; ++i) {
        QString name = nameList.at(i);
        ElementWidget* curWidget = widgetList.at(i);
        combinedPair.push_back(std::make_pair(name, curWidget));
    }
    return combinedPair;
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::handleRename(int index, const QString& newName)
{
    Q_ASSERT(index >= 0 && index < nameList.size());
    nameList[index] = newName;
    // we keep a pointer here and push the notification after all the invariants are recovered
    ElementWidget* w = widgetList.at(index);
    int numElements = nameList.size();
    QVector<std::pair<QString, ElementWidget*>> combinedPair = getCombinedPairVec(numElements);
    NameSorting::sortNameListWithData(combinedPair);
    int curIndex = -1;
    for (int i = 0; i < numElements; ++i) {
        auto& p = combinedPair.at(i);
        nameList[i] = p.first;
        widgetList[i] = p.second;
        if (p.second == w) {
            curIndex = i;
        }
    }
    notifyNameChange(newName, w);
    nameListUpdated();
    Q_ASSERT(curIndex >= 0);
    listWidget->setCurrentRow(curIndex);
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::handleDelete(int index)
{
    Q_ASSERT(index >= 0 && index < widgetList.size());
    ElementWidget* currentWidget = widgetList.at(index);
    int nextIndex = listWidget->currentRow();
    if (stackedWidget->currentWidget() == currentWidget) {
        if (widgetList.size() > 1) {
            nextIndex = (index == widgetList.size()-1)? index - 1 : index;
        } else {
            nextIndex = -1;
        }
    }
    stackedWidget->removeWidget(currentWidget);
    currentWidget->deleteLater();
    widgetList.removeAt(index);
    nameList.removeAt(index);
    nameListUpdated();
    listWidget->setCurrentRow(nextIndex);
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::handleAddElement(const QString& name, ElementWidget* newWidget)
{
    QVector<std::pair<QString, ElementWidget*>> combinedPairVec = getCombinedPairVec(nameList.size() + 1);
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
    nameListUpdated();
    Q_ASSERT(curIndex >= 0);
    listWidget->setCurrentRow(curIndex);
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::handleNew(const QString& name)
{
    // because beside the call from right click menu, this function is also called by incoming signal connection,
    // we need to check if the name is already used, and just early exit if it is the case.
    if (isElementExist(name))
        return;

    ElementWidget* newWidget = createWidgetWithData(name);
    handleAddElement(name, newWidget);
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::handleCopyAs(const QString& name, const QString& srcName, ElementWidget* src)
{
    ElementWidget* newWidget = createWidgetWithDataFromSource(name, srcName, src);
    handleAddElement(name, newWidget);
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::listWidgetContextMenuEventHandler(const QPoint& pos)
{
    QListWidgetItem* item = listWidget->itemAt(pos);
    QMenu menu(listWidget);
    QString oldName;
    if (item) {
        int row = listWidget->row(item);
        Q_ASSERT(row >= 0 && row < nameList.size());
        oldName = nameList.at(row);
        Q_ASSERT(oldName == item->text());

        QAction* renameAction = new QAction(tr("Rename"));
        QObject::connect(renameAction, &QAction::triggered, getObj(), [=](){
            QString newName = getNameEditDialog(oldName, false);
            if (newName.isEmpty() || newName == oldName)
                return;
            if (isElementExist(newName)) {
                QMessageBox::warning(dialogParent, tr("Rename failed"), tr("There is already an entry named \"%1\".").arg(newName));
                tryGoToElement(newName);
                return;
            }
            handleRename(row, newName);
        });
        menu.addAction(renameAction);

        QAction* copyAsAction = new QAction(tr("Copy As"));
        QObject::connect(copyAsAction, &QAction::triggered, getObj(), [=](){
            QString newName = getNameEditDialog(oldName, true);
            if (newName.isEmpty() || newName == oldName)
                return;
            if (isElementExist(newName)) {
                QMessageBox::warning(dialogParent, tr("Copy failed"), tr("There is already an entry named \"%1\".").arg(newName));
                tryGoToElement(newName);
                return;
            }
            handleCopyAs(newName, oldName, widgetList.at(row));
        });
        menu.addAction(copyAsAction);

        QAction* deleteAction = new QAction(tr("Delete"));
        QObject::connect(deleteAction, &QAction::triggered, getObj(), [=](){
            if (QMessageBox::question(dialogParent,
                                      tr("Delete confirmation"),
                                      tr("Are you sure you want to delete \"%1\"?").arg(oldName),
                                      QMessageBox::Yes | QMessageBox::Cancel) == QMessageBox::Yes) {
                handleDelete(row);
            }
        });
        menu.addAction(deleteAction);
    }
    QAction* newAction = new QAction(tr("New"));
    QObject::connect(newAction, &QAction::triggered, getObj(), [=](){
        QString newName = getNameEditDialog(oldName, true);
        if (newName.isEmpty() || newName == oldName)
            return;
        // if the name is already in use, pop up a dialog
        if (isElementExist(newName)) {
            QMessageBox::warning(dialogParent, tr("Creation failed"), tr("There is already an entry named \"%1\".").arg(newName));
            tryGoToElement(newName);
            return;
        }
        handleNew(newName);
    });
    menu.addAction(newAction);
    menu.exec(listWidget->mapToGlobal(pos));
}

template <typename ElementWidget, bool isSortElementByName>
QString NamedElementListController<ElementWidget, isSortElementByName>::getNameEditDialog(const QString& oldName, bool isNewInsteadofRename)
{
    QString title;
    QString prompt;
    if (isNewInsteadofRename) {
        title = NamedElementListControllerObject::tr("New");
        prompt = NamedElementListControllerObject::tr("Please input the name:");
    } else {
        title = NamedElementListControllerObject::tr("Rename");
        prompt = NamedElementListControllerObject::tr("Please input the new name for \"%1\":").arg(oldName);
    }
    bool ok = false;
    QString result = QInputDialog::getText(dialogParent, title, prompt, QLineEdit::Normal, oldName, &ok);
    if (ok) {
        return result;
    }
    return QString();
}

#endif // NAMEDELEMENTLISTCONTROLLER_H
