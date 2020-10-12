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
    struct DataObjectInfo {
        QString name; //!< name to be used for widget title; empty for main input or main output
        bool isOutputInsteadOfInput; //!< true for output, false for input
    };

public:
    virtual ~TransformPassViewDelegateObject() = default;

    // default to 1 input (#0) and 1 output(#1)
    virtual int getNumDataObjects() const {return 2;}

    virtual DataObjectInfo getDataInfo(int objectID) const {
        DataObjectInfo info;
        info.isOutputInsteadOfInput = false;
        switch (objectID) {
        default: Q_UNREACHABLE();
        case 0: break;
        case 1:
            info.isOutputInsteadOfInput = true;
        }
        return info;
    }

    // it should take care of signal-slot connections
    virtual QWidget* getDataWidget(TransformPassViewWidget* w, int objectID) = 0;

    // called when dataReady() slot is invoked
    virtual void updateDataWidgetForNewData() {}

    virtual std::function<bool(const QVariant&, const QVariant&)> getDataLocationComparator(int objectID) const {
        Q_UNUSED(objectID);
        return [](const QVariant& lhs, const QVariant& rhs) -> bool {
            return lhs.toInt() < rhs.toInt();
        };
    }

    virtual QString getLocationDescriptionString(int objectID, const QVariant& locData) const {
        Q_UNUSED(objectID)
        Q_UNUSED(locData)
        return QStringLiteral("<Unimplemented>");
    }

    // note that currently we always show event coloring as background color
    // reset coloring (restore original background color) for input widget
    virtual void resetDataWidgetEventColoring(int objectID) {Q_UNUSED(objectID)}
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
    QHash<int, EventLocationMapType> dataStartPosMap;
    QVector<QColor> activelyColoredEventColors;
    EventFocusType eventFocusTy = EventFocusType::NoFocus;
    QVariant eventFocusLocation; // where is the focus of event
    int lastDetailedEventIndex = -2; // what's the current event's index, whose details are listed at bottom-right panel
    QHash<QTreeWidgetItem*, std::pair<int, QVariant>> currentEventLocations; // locations for all objects recorded in current events
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

    static QString getEventNameLabel(const EventLogger *logger, const EventInterpreter* interp, int eventIndex);

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
