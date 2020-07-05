#ifndef TRANSFORMPASSVIEWWIDGET_H
#define TRANSFORMPASSVIEWWIDGET_H

#include "src/lib/Tree/EventLogging.h"

#include <QWidget>
#include <QVBoxLayout>

#include <functional>
#include <map>
#include <memory>

namespace Ui {
class TransformPassViewWidget;
}

class TransformPassViewWidget;
class TransformPassViewDelegateObject
{
public:
    virtual ~TransformPassViewDelegateObject() = default;

    // they should take care of signal-slot connections
    virtual QWidget* getInputDataWidget(TransformPassViewWidget* w) = 0;
    virtual QWidget* getOutputDataWidget(TransformPassViewWidget* w) = 0;

    // called when dataReady() slot is invoked
    virtual void updateDataWidgetForNewData() {}

    virtual std::function<bool(const QVariant&, const QVariant&)> getInputDataPositionComparator() const {
        return [](const QVariant& lhs, const QVariant& rhs) -> bool {
            return lhs.toInt() < rhs.toInt();
        };
    }

    virtual std::function<bool(const QVariant&, const QVariant&)> getOutputDataPositionComparator() const {
        return [](const QVariant& lhs, const QVariant& rhs) -> bool {
            return lhs.toInt() < rhs.toInt();
        };
    }
};

class TransformPassViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TransformPassViewWidget(QWidget *parent = nullptr);

    void installDelegateObject(TransformPassViewDelegateObject* obj);

    ~TransformPassViewWidget();

public slots:
    // connection with ExecuteObject
    void transformRestarted();
    void dataReady(EventLogger* e);

    // connection with input/output data widget
    // query events where only the "start" location is in [start, end)
    void inputDataEventRangeLookupRequested (const QVariant& start, const QVariant& end);
    void outputDataEventRangeLookupRequested(const QVariant& start, const QVariant& end);
    // query events whose [start, end) includes "pos"
    void inputDataEventSinglePositionLookupRequested (const QVariant& pos);
    void outputDataEventSinglePositionLookupRequested(const QVariant& pos);

private:
    // start -> [end, event index]; null end if this is a "point" event instead of range event
    using EventLocationMapType = std::multimap<QVariant, std::pair<QVariant,int>, std::function<bool(const QVariant&, const QVariant&)>>;

    void buildEventLocationMap();

private:
    Ui::TransformPassViewWidget *ui;
    TransformPassViewDelegateObject* delegateObject = nullptr;
    std::unique_ptr<EventLogger> events;
    QWidget* inputDataWidget = nullptr;
    QWidget* outputDataWidget = nullptr;
    QVBoxLayout* inputDataGroupBoxLayout = nullptr;
    QVBoxLayout* outputDataGroupBoxLayout = nullptr;
    std::unique_ptr<EventLocationMapType> inputDataStartPosMap;
    std::unique_ptr<EventLocationMapType> outputDataStartPosMap;
};

#endif // TRANSFORMPASSVIEWWIDGET_H
