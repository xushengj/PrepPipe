#ifndef ANONYMOUSELEMENTLISTCONTROLLER_H
#define ANONYMOUSELEMENTLISTCONTROLLER_H

#include <QObject>
#include <QListWidget>
#include <QStackedWidget>
#include <QList>
#include <QHash>
#include <QMap>
#include <QStringList>

#include <type_traits>

class AnonymousElementListControllerObject: public QObject
{
    Q_OBJECT
public:
    template <typename ElementWidget>
    friend class AnonymousElementListController;

// --------------------------------------------------------
// signal / slots exposed to outside
signals:
    void dirty();

// --------------------------------------------------------
// those that are not exposed (internal use in controller)

private slots:
    void currentElementChangeHandler(int index);

private:
    // only the controller can instantiate it
    AnonymousElementListControllerObject();
    void init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg);
    void setCurrentElementChangeCallback(std::function<void(int)> cb) {
        currentElementChangeCB = cb;
    }

private:
    QListWidget* listWidget = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    std::function<void(int)> currentElementChangeCB;
};

template <typename ElementWidget>
class AnonymousElementListController
{
    static_assert (std::is_base_of<QWidget, ElementWidget>::value, "ElementWidget must inherit QWidget!");
    static_assert (std::is_class<typename ElementWidget::StorageData>::value, "Element Widget must have a public struct member called StorageData!");

    // the ElementWidget must have the following member functions:
    // void setData(const StorageData&);
    // void getData(StorageData&);
    // QString getDisplayLabel();

public:
    using ElementStorageData = typename ElementWidget::StorageData;

public:
    AnonymousElementListController() = default;

    void init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg);

    void setCreateWidgetCallback(std::function<ElementWidget*(AnonymousElementListControllerObject*)> cb) {
        createWidgetCallback = cb;
    }

    AnonymousElementListControllerObject* getObj() {
        return &obj;
    }

    void setData(const QVector<ElementStorageData>& data);
    void getData(QVector<ElementStorageData>& data);

private:
    // private functions that show how ElementWidget would be used
    void addElement(ElementWidget* widget) {
        QString name = widget->getDisplayLabel();
        widgetList.push_back(widget);
        QObject::connect(widget, &ElementWidget::dirty, &obj, &AnonymousElementListControllerObject::dirty);
        listWidget->addItem(name);
        stackedWidget->addWidget(widget);
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

    void clearData();

private:
    AnonymousElementListControllerObject obj;
    QListWidget* listWidget = nullptr;
    QStackedWidget* stackedWidget = nullptr;
    std::function<ElementWidget*(AnonymousElementListControllerObject*)> createWidgetCallback;
    QWidget* placeHolderPage = nullptr;
    QList<ElementWidget*> widgetList;
};

template <typename ElementWidget>
void AnonymousElementListController<ElementWidget>::init(QListWidget* listWidgetArg, QStackedWidget* stackedWidgetArg)
{
    listWidget = listWidgetArg;
    stackedWidget = stackedWidgetArg;

    // there must be a place holder page / widget in the stack widget and nothing else should be there
    Q_ASSERT(stackedWidget->count() == 1);
    placeHolderPage = stackedWidget->currentWidget();

    obj.init(listWidgetArg, stackedWidgetArg);

    obj.setCurrentElementChangeCallback(std::bind(&AnonymousElementListController<ElementWidget>::currentElementChangedHandler, this, std::placeholders::_1));
}

template <typename ElementWidget>
void AnonymousElementListController<ElementWidget>::clearData()
{
    if (widgetList.isEmpty())
        return;

    stackedWidget->setCurrentWidget(placeHolderPage);

    for (QWidget* editor : widgetList) {
        stackedWidget->removeWidget(editor);
        editor->deleteLater();
    }

    widgetList.clear();
}

template <typename ElementWidget>
void AnonymousElementListController<ElementWidget>::setData(const QVector<ElementStorageData>& data)
{
    clearData();
    int numElements = data.size();
    for (int i = 0; i < numElements; ++i) {
        const ElementStorageData& storage = data.at(i);
        ElementWidget* widget = createWidget();
        widget->setData(storage);
        addElement(widget);
    }
    if (numElements > 0) {
        stackedWidget->setCurrentWidget(widgetList.front());
    }
}

template <typename ElementWidget>
void AnonymousElementListController<ElementWidget>::getData(QVector<ElementStorageData>& data)
{
    int numElements = widgetList.size();
    data.resize(numElements);
    for (int i = 0; i < numElements; ++i) {
        widgetList.at(i)->getData(data[i]);
    }
}

#endif // ANONYMOUSELEMENTLISTCONTROLLER_H
