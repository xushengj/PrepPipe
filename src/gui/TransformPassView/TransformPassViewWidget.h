#ifndef TRANSFORMPASSVIEWWIDGET_H
#define TRANSFORMPASSVIEWWIDGET_H

#include "src/lib/Tree/EventLogging.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QAbstractListModel>
#include <QTreeWidgetItem>

#include <functional>
#include <map>
#include <memory>

namespace Ui {
class TransformPassViewWidget;
}

class TransformPassViewWidget;
class TransformEventListModel;
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

    // note that currently we always show event coloring as background color
    // reset coloring (restore original background color) for input widget
    virtual void resetInputDataWidgetEventColoring() {}
    virtual void resetOutputDataWidgetEventColoring() {}
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

    void eventRecolorRequested(); // recompute colors for all
    void unsetEventFocus();

    void setFocusToEvent(int eventIndex);

private slots:
    void updateEventDetails(int eventIndex);
    void handleEventListSelectionChange(const QModelIndex& current, const QModelIndex& prev);
    void handleEventClickableItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    // start -> [end, event index]; null end if this is a "point" event instead of range event
    using EventLocationMapType = std::multimap<QVariant, std::pair<QVariant,int>, std::function<bool(const QVariant&, const QVariant&)>>;

    // where is the event focus
    enum class EventFocusType {
        NoFocus, // eventFocusLocation is invalid
        Event, // eventFocusLocation is an integer containing the focus event index
        Input, // eventFocusLocation is an (from this class's perspective) opaque data that describe a location in input data
        Output // eventFocusLocation is an (from this class's perspective) opaque data that describe a location in output data
    };

    // helper struct to extract event locations
    struct EventLocationExpansionData {
        QVariant inputDataStart;
        QVariant inputDataEnd;
        QVariant outputDataStart;
        QVariant outputDataEnd;
        bool inputPosSet = false;
        bool outputPosSet = false;
    };

    static EventLocationExpansionData extractLocationsFromEvent(const Event& e);
    void buildEventLocationMap();
    void requestRefreshingHighlighting();
    void refreshHighlighting(); // Don't call this directly; call requestRefreshingHighlighting() instead

private:
    Ui::TransformPassViewWidget *ui;
    std::unique_ptr<TransformPassViewDelegateObject> delegateObject;
    std::unique_ptr<EventLogger> events;
    QWidget* inputDataWidget = nullptr;
    QWidget* outputDataWidget = nullptr;
    QVBoxLayout* inputDataGroupBoxLayout = nullptr;
    QVBoxLayout* outputDataGroupBoxLayout = nullptr;
    std::unique_ptr<EventLocationMapType> inputDataStartPosMap;
    std::unique_ptr<EventLocationMapType> outputDataStartPosMap;
    QVector<QColor> activelyColoredEventColors;
    EventFocusType eventFocusTy = EventFocusType::NoFocus;
    QVariant eventFocusLocation; // where is the focus of event
    int lastDetailedEventIndex = -2; // what's the current event's index, whose details are listed at bottom-right panel
    QHash<QTreeWidgetItem*, QVariant> currentEventInputPositions;
    QHash<QTreeWidgetItem*, QVariant> currentEventOutputPositions;
    QHash<QTreeWidgetItem*, int> currentEventCrossReferences; // both events referencing current event and the events being referenced by current event

    // [eventIndex] -> vec of <eventIndex, referenceTy>
    // for each event, what other events has referenced them, with what reference type. same size as events
    QVector<QVector<std::pair<int,int>>> eventIncomingReferences;

    bool isRefreshRequested = false;
    TransformEventListModel* eventListModel = nullptr;
};

class TransformEventListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    // these calls do not take the ownership of object
    TransformEventListModel(QObject* parent);
    void setLogger(EventLogger* logger);
    void setFilter(std::function<bool(EventLogger*,int)> filterCB);

    bool isHidingSecondaryEvent() const {
        return isHideSecondaryEvents;
    }
    QModelIndex getModelIndexFromEventIndex(int eventIndex) const;
    int getEventIndexFromListIndex(int listIndex) const;
    int getNumEventInList() const;

    static QString getEventNameLabel(int eventIndex, const EventInterpreter* interp, const Event& e);

public:
    // data output
    // each row is "[index] event short name"
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    // secondary event: events that are reachable by reference from other events
    void toggleHideSecondaryEvents(bool isHide);

private:
    void recomputeFilteredEvents(std::function<bool(EventLogger*,int)> filterCB = std::function<bool(EventLogger*,int)>());

private:
    EventLogger* logger = nullptr;

    bool isHideSecondaryEvents = true;

    // sorted if non-empty; empty if all events are in the set.
    // This is the result AFTER filter is applied but BEFORE considering whether to hide secondary events
    QVector<int> filteredEventVec;

    // This is the result AFTER hiding secondary events
    QVector<int> topLevelFinalEventVec;

};

#endif // TRANSFORMPASSVIEWWIDGET_H
