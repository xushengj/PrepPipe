#ifndef EVENTLOGGING_H
#define EVENTLOGGING_H

#include <QList>
#include <QStringList>
#include <QVariantList>

struct EventReference {
    int referenceTypeIndex  = -1; //!< This should be a casted enum value
    int eventIndex          = -1;
};

enum class EventLocationType : int {
    // if an event location is in following types,
    // it will be used when searching for events from locations.
    // if an event happens at a point instead of a range, then the "end" location is not necessary.
    InputDataStart = 0,
    InputDataEnd,
    OutputDataStart,
    OutputDataEnd,
    // other locations that can be navigated to but is not used when searching for events
    OTHER_START
};

struct EventLocationRemark {
    int locationTypeIndex = -1; //!< This should be a casted enum value from EventReference, or the ones starting from OTHER_START.
    QVariant location;
};

struct Event {
    int eventTypeIndex  = -1; //!< This should be a casted enum value
    int selfIndex       = -1;
    QVector<EventReference> references;
    QVector<EventLocationRemark> locationRemarks;
    QVariantList data;
};

// the derived class should be static global variables that get passed into EventLogger's constructor
class EventInterpreter {
protected:
    EventInterpreter() = default;

public:
    virtual ~EventInterpreter() = default;

    virtual QString getEventTypeTitle(int eventTypeIndex) const {
        return QString::number(eventTypeIndex);
    }

    virtual QString getDetailString(const Event& e) const {
        return getEventTypeTitle(e.eventTypeIndex);
    }

    virtual QString getReferenceTypeTitle(int eventTypeIndex, int referenceTypeIndex) const {
        Q_UNUSED(eventTypeIndex)
        return QString::number(referenceTypeIndex);
    }
    virtual QString getLocationTypeTitle(int eventTypeIndex, int locationTypeIndex) const {
        Q_UNUSED(eventTypeIndex)
        return QString::number(locationTypeIndex);
    }
};

class EventLogger {
public:
    EventLogger() = default;
    EventLogger(const EventLogger&) = default;
    EventLogger(EventLogger&&) = default;

    EventLogger* clone() const {
        return new EventLogger(*this);
    }

    int addEvent(int typeIndex, const QVector<EventReference>& references, const QVector<EventLocationRemark>& locationRemarks, const QVariantList& data);
    const Event& getEvent(int index) const {return events.at(index);}
    int size() const {return events.size();}
private:
    QVector<Event> events;
};

#endif // EVENTLOGGING_H
