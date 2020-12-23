#ifndef EVENTLOGGING_H
#define EVENTLOGGING_H

#include <QList>
#include <QStringList>
#include <QVariantList>
#include <QMetaEnum>
#include <QDebug>

#include <type_traits>

/* Event logging
 *
 * In a pass-based, data-oriented pipeline, we want to capture events happened during a pass to facilitate post-mortem debugging
 * instead of using traditional, single stepping based debugging. To some extent it is a more automated form of printf() debugging.
 *
 * We will want to capture following semantics into our log to ease debugging:
 * 1. mapping between locations (mainly input to output)
 *    this is crucial for identifying where goes wrong during the pass efficiently, both experts and new users.
 * 2. (cause and effect) relationships between events
 *    traditional "flat" logging discards this semantics, which makes new users confused on "what error is important" or even "what is the error".
 *    (In my personal experience, I got many students asking why "make xxx" fails, without showing the log before the last error message...)
 *
 * The first is modeled in EventLocationRemark struct and the second is modeled in EventReference struct. The complete data for a event is in Event struct.
 */

struct EventReference {
    int referenceTypeIndex  = -1; //!< This should be a casted enum value
                                  //!< The value should be ordered such that the ones more important should be smaller,
                                  //!< while the ones less important should be larger.
    int eventIndex          = -1;
    int referenceSortOrder  =  0; //!< A number indicating what's the importance level of referred event that
                                  //!< lead to current event under the same reference type
                                  //!< a number with smaller absolute value would make the reference appear earlier in the list of
                                  //!< referred events
                                  //!< a negative number will make the reference to be displayed in a grayed-out
                                  //!< style, intended for negative events
                                  //!< (e.g. when justifying why a pattern is selected, the event that another
                                  //!< pattern failed should be given negative sort order)
                                  //!< (We are bringing supportive / negative event distinction semantics to our general handling logic)

    // used for sorting EventReference in containers
    bool operator<(const EventReference& rhs) const {
        // compare referenceTypeIndex first
        if (referenceTypeIndex < rhs.referenceTypeIndex) return true;
        if (referenceTypeIndex > rhs.referenceTypeIndex) return false;

        // compare referenceSortOrder next
        // non-negative ones is "smaller", then negative ones
        // when both are non-negative or both are negative, then the absolute value decides the order; smaller absolute value is smaller
        if (referenceSortOrder >= 0 && rhs.referenceSortOrder >= 0) {
            if (referenceSortOrder < rhs.referenceSortOrder) return true;
            if (referenceSortOrder > rhs.referenceSortOrder) return false;
        } else if (referenceSortOrder < 0 && rhs.referenceSortOrder < 0) {
            // inversed value from the case above
            if (referenceSortOrder < rhs.referenceSortOrder) return false;
            if (referenceSortOrder > rhs.referenceSortOrder) return true;
        } else {
            // if we reached here then the signs of two sort order are different
            return (referenceSortOrder >= 0);
        }

        // finally, eventIndex
        return eventIndex < rhs.eventIndex;
    }

    static EventReference makeReference(int tyIndex, int eventIndex, int sortOrder = 0) {
        EventReference ref;
        ref.referenceTypeIndex = tyIndex;
        ref.eventIndex = eventIndex;
        ref.referenceSortOrder = sortOrder;
        return ref;
    }

    template<typename EnumTy>
    static EventReference makeReference(EnumTy tyIndex, int eventIndex, int sortOrder = 0) {
        static_assert (std::is_enum<EnumTy>::value, "EnumTy should be enum");
        return makeReference(static_cast<int>(tyIndex), eventIndex, sortOrder);
    }

    template<typename ContainerTy, typename EnumTy>
    static void addReference(ContainerTy& container, EnumTy tyIndex, int eventIndex, int sortOrder = 0) {
        static_assert (std::is_enum<EnumTy>::value, "EnumTy should be enum");
        container.push_back(makeReference(static_cast<int>(tyIndex), eventIndex, sortOrder));
    }

    template<typename ContainerTy, typename EnumTy>
    static void addReference(ContainerTy& container, EnumTy tyIndex, const QList<int>& positiveEvents, const QList<int>& negativeEvents) {
        static_assert (std::is_enum<EnumTy>::value, "EnumTy should be enum");
        for (int i = 0, n = positiveEvents.size(); i < n; ++i) {
            EventReference::addReference(container, tyIndex, positiveEvents.at(i), i);
        }
        for (int i = 0, n = negativeEvents.size(); i < n; ++i) {
            EventReference::addReference(container, tyIndex, negativeEvents.at(i), -1-i);
        }
    }
};

inline QDebug operator<<(QDebug debug, const EventReference& e)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << e.referenceTypeIndex << ':' << e.eventIndex << '@' << e.referenceSortOrder << ')';
    return debug;
}

/// This option determines how the coloring (our way of showing event location to user) of the event is done
/// an option of higher number imply coloring condition in lower number (i.e. if an event is Actively colored, it is also colored when user highlight a referencing event)
/// Note that if the event do not have any location remarks, then it is never colored
/// Each option is only for the corresponding event only; no assumption on color option of referred / referring events
enum class EventColorOption : int {
    Passive = 0,        //!< this event should only colored when the user is highlighting it directly
    Referable = 1,      //!< this event is highlighted if being referenced by the current highlighted event (only considering direct reference)
    Active = 2,         //!< this event is always colored unless the user is highlighting other events (it represents the "first level" correlation between input/output)
};

struct EventLocationRemark {
    QVariant location;
    int locationContextIndex = 0; //!< This indicates which context is the location for, for example whether this is a location in an input object or an output object
};

inline QDebug operator<<(QDebug debug, const EventLocationRemark& l)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << l.locationContextIndex << ':' << l.location << ')';
    return debug;
}

struct Event {
    int eventTypeIndex      = -1; //!< This should be a casted enum value
    int selfIndex           = -1;
    int activeColorIndex    = -1; //!< color index if this event is actively colored (other color indices are always generated on-demand)
    EventColorOption colorOption = EventColorOption::Referable;
    QVector<EventReference> references;
    QVector<EventLocationRemark> locationRemarks;
    QVariantList data;
};

// the derived class should be static global variables that get passed into EventLogger's constructor
class EventLogger;
class EventInterpreter {
public:
    enum class EventImportance {
        AlwaysPrimary,   // the event is always a primary event that should not be hidden by "hide secondary events"
        Auto,            // the event is considered a primary event if no other events reference it
        AlwaysSecondary, // the event is always a secondary event, even if no other events reference it
    };

    EventInterpreter() = default;
    virtual ~EventInterpreter() = default;

    virtual EventImportance getEventImportance (int eventTypeIndex) const {
        Q_UNUSED(eventTypeIndex);
        return EventImportance::Auto;
    }

    // functions to get a text describing each entity
    // all the information needed for a descriptive text should be completely contained in statically known data and the event logger
    // if any information in input/output data is needed, they should be included inside the event's data
    virtual QString getEventTitle           (const EventLogger* logger, int eventIndex, int eventTypeIndex) const;
    virtual QString getDetailString         (const EventLogger* logger, int eventIndex) const;
    virtual QString getReferenceTypeTitle   (const EventLogger* logger, int eventIndex, int eventTypeIndex, int referenceTypeIndex) const;

    // return empty string if there is no need for a location type string (e.g., when the event only have 1 location)
    virtual QString getLocationTypeTitle    (const EventLogger* logger, int eventIndex, int eventTypeIndex, int locationIndex) const;
};

class DefaultEventInterpreter: public EventInterpreter
{
public:
    DefaultEventInterpreter() = default;
    virtual ~DefaultEventInterpreter() = default;

    void initialize(QMetaEnum eventIDMetaArg, QMetaEnum eventReferenceIDMetaArg, QMetaEnum eventLocationIDMetaArg) {
          eventIDMeta = eventIDMetaArg;
          eventReferenceIDMeta = eventReferenceIDMetaArg;
          eventLocationIDMeta = eventLocationIDMetaArg;
    }

    bool isValid() const {return eventIDMeta.isValid();}

    virtual QString getEventTitle           (const EventLogger* logger, int eventIndex, int eventTypeIndex) const override;
    virtual QString getReferenceTypeTitle   (const EventLogger* logger, int eventIndex, int eventTypeIndex, int referenceTypeIndex) const override;

private:
    QMetaEnum eventIDMeta;
    QMetaEnum eventReferenceIDMeta;
    QMetaEnum eventLocationIDMeta;
};

class EventLogger {
public:
    EventLogger() = default;
    EventLogger(const EventLogger&) = default;
    EventLogger(EventLogger&&) = default;

    EventLogger* clone() const {
        return new EventLogger(*this);
    }

    void installInterpreter(const EventInterpreter* interpArg) {
        interp = interpArg;
    }
    const EventInterpreter* getInterpreter() const {
        return interp;
    }

    int addEvent(int typeIndex, const QVariantList& data,
                 EventColorOption colorOption,
                 const QVector<EventReference>& references,
                 const QVector<EventLocationRemark>& locationRemarks
    );
    // functions to append additional information after the event is registered
    // event references cannot be appended because it may be sorted upon registration, and this also prevents circles in event reference
    void appendEventData(int eventIndex, const QVariantList& data) {
        Q_ASSERT(eventIndex >= 0 && eventIndex < events.size());
        events[eventIndex].data.append(data);
    }
    void appendEventData(int eventIndex, const QVariant& data) {
        Q_ASSERT(eventIndex >= 0 && eventIndex < events.size());
        events[eventIndex].data.append(data);
    }
    void appendEventLocationRemarks(int eventIndex, const QVector<EventLocationRemark>& locationRemarks) {
        Q_ASSERT(eventIndex >= 0 && eventIndex < events.size());
        events[eventIndex].locationRemarks.append(locationRemarks);
    }
    void appendEventLocationRemarks(int eventIndex, const EventLocationRemark& locationRemark) {
        Q_ASSERT(eventIndex >= 0 && eventIndex < events.size());
        events[eventIndex].locationRemarks.append(locationRemark);
    }

    // the variant that does the cast
    template<typename IndexTy>
    int addEvent(IndexTy typeIndex, const QVariantList& data = QVariantList(),
                 EventColorOption colorOption = EventColorOption::Referable,
                 const QVector<EventReference>& references = QVector<EventReference>(),
                 const QVector<EventLocationRemark>& locationRemarks = QVector<EventLocationRemark>()
    ) {
        static_assert (std::is_enum<IndexTy>::value, "IndexTy should be enum");
        return addEvent(static_cast<int>(typeIndex), data, colorOption, references, locationRemarks);
    }

    const Event& getEvent(int index) const {return events.at(index);}
    int size() const {return events.size();}
    int getNumActivelyColoredEvents() const {return numActivelyColoredEvents;}

    void passFailed(int firstFatalEventIndex) {
        Q_ASSERT(fatalEventIndex == -2);
        Q_ASSERT(firstFatalEventIndex >= 0 && firstFatalEventIndex < events.size());
        fatalEventIndex = firstFatalEventIndex;
    }

    void passSucceeded() {
        Q_ASSERT(fatalEventIndex == -2);
        fatalEventIndex = -1;
    }

    bool isPassFailed() const {return (fatalEventIndex >= 0);}
    bool isPassSucceeded() const {return (fatalEventIndex == -1);}
    int getFatalEventIndex() const {return fatalEventIndex;}

private:
    static EventInterpreter defaultInterp;

private:
    const EventInterpreter* interp = &defaultInterp; // not taking ownership
    QVector<Event> events;
    int numActivelyColoredEvents = 0;
    int fatalEventIndex = -2; // -2: unknown whether failed or not; -1: success; >=0: first fatal event causing the failure
};

#endif // EVENTLOGGING_H
