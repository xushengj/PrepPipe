#ifndef NAMEDELEMENTLISTCONTROLLER_H
#define NAMEDELEMENTLISTCONTROLLER_H

#include <QObject>
#include <QListWidget>
#include <QStackedWidget>
#include <QList>
#include <QHash>
#include <QMap>
#include <QStringList>

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

public slots:
    void tryGoToElement(const QString& elementName);

// --------------------------------------------------------
// those that are not exposed (internal use in controller)

private slots:
    void currentElementChangeHandler(int index);
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

private:
    QListWidget* listWidget = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    std::function<void(int)> currentElementChangeCB;
    std::function<void(const QString&)> gotoElementCB;
};

template <typename ElementWidget, bool isSortElementByName>
class NamedElementListController {
    static_assert (std::is_base_of<QWidget, ElementWidget>::value, "ElementWidget must inherit QWidget!");
    static_assert (std::is_class<typename ElementWidget::StorageData>::value, "Element Widget must have a public struct member called StorageData!");
    // ElementWidget can optionally have a public struct member caled HelperData, which represent the gui helper data that is not present in base object but in GUI object

    // ElementWidget should have the following member functions:
    // void setData(const QString& name, const StorageData& storageData, const HelperData& helperData);
    // void getData(StorageData&, HelperData&);

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
    void addElement(const QString& name, ElementWidget* widget) {
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

    // other (ElementWidget type agnostic) helper functions
    void clearData();
    void tryGoToElement(const QString& element);


private:
    NamedElementListControllerObject obj;
    QListWidget* listWidget = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    std::function<QString(const ElementStorageData&)> getNameCallback;
    std::function<ElementWidget*(NamedElementListControllerObject*)> createWidgetCallback;

    QWidget* placeHolderPage = nullptr;
    QHash<QString, int> nameSearchMap; // Name -> index in nameList
    QStringList nameList;
    QList<ElementWidget*> widgetList;
};

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg)
{
    listWidget = listWidgetArg;
    stackedWidget = stackedWidgetArg;

    // there must be a place holder page / widget in the stack widget and nothing else should be there
    Q_ASSERT(stackedWidget->count() == 1);
    placeHolderPage = stackedWidget->currentWidget();

    obj.init(listWidgetArg, stackedWidgetArg);

    obj.setCurrentElementChangeCallback(std::bind(&NamedElementListController<ElementWidget, isSortElementByName>::currentElementChangedHandler, this, std::placeholders::_1));
    obj.setGotoElementCallback(std::bind(&NamedElementListController<ElementWidget, isSortElementByName>::tryGoToElement, this, std::placeholders::_1));
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
}

template <typename ElementWidget, bool isSortElementByName>
void NamedElementListController<ElementWidget, isSortElementByName>::tryGoToElement(const QString& element)
{
    int index = nameSearchMap.value(element, -1);

    // this would also emit signal that cause stackedWidget to be updated
    listWidget->setCurrentRow(index);
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
        addElement(name, widget);
    }
    if (numElements > 0) {
        stackedWidget->setCurrentWidget(widgetList.front());
    }
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
        widget->setData(data[i], helperData[i]);
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
            addElement(name, widget);
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
            addElement(name, widget);
        }
    }
    if (numElements > 0) {
        stackedWidget->setCurrentWidget(widgetList.front());
    }
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
        widget->setData(storage, helper);
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
        addElement(name, widget);
    }
    if (numElements > 0) {
        stackedWidget->setCurrentWidget(widgetList.front());
    }
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
        widget->setData(data[i]);
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
            addElement(name, widget);
        }
    } else {
        for (auto iterData = data.begin(), iterDataEnd = data.end(); iterData != iterDataEnd; ++iterData) {
            const QString& name = iterData.key();
            const ElementStorageData& storage = iterData.value();
            ElementWidget* widget = createWidget();
            widget->setData(name, storage);
            addElement(name, widget);
        }
    }
    if (numElements > 0) {
        stackedWidget->setCurrentWidget(widgetList.front());
    }
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
        widget->setData(storage);
        data.insert(name, storage);
    }
}

#endif // NAMEDELEMENTLISTCONTROLLER_H
