#ifndef EVENTLOGGING_H
#define EVENTLOGGING_H

#include <QList>
#include <QStringList>
#include <QVariantList>

#include <type_traits>

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

enum class EventLocationType : int {
    // if an event location is in following types,
    // it will be used when searching for events from locations.
    // if an event happens at a point instead of a range, then the "end" location is not necessary / should be avoided.
    InputDataStart = 0,
    InputDataEnd,
    OutputDataStart,
    OutputDataEnd,
    // other locations that can be navigated to but is not used when searching for events
    OTHER_START
};

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
    int locationTypeIndex = -1; //!< This should be a casted enum value from EventReference, or the ones starting from OTHER_START.
    QVariant location;

    static EventLocationRemark makeRemark(int tyIndex, const QVariant& loc)
    {
        EventLocationRemark remark;
        remark.locationTypeIndex = tyIndex;
        remark.location = loc;
        return remark;
    }

    template<typename EnumTy>
    static EventLocationRemark makeRemark(EnumTy tyIndex, const QVariant& loc)
    {
        static_assert (std::is_enum<EnumTy>::value, "EnumTy should be enum");
        return makeRemark(static_cast<int>(tyIndex), loc);
    }

    template<typename ContainerTy, typename EnumTy>
    static void addRemark(ContainerTy& container, EnumTy tyIndex, const QVariant& loc)
    {
        static_assert (std::is_enum<EnumTy>::value, "EnumTy should be enum");
        container.push_back(makeRemark(static_cast<int>(tyIndex), loc));
    }
};

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

    int addEvent(int typeIndex, const QVariantList& data,
                 EventColorOption colorOption,
                 const QVector<EventReference>& references,
                 const QVector<EventLocationRemark>& locationRemarks
    );

    // the variant that does the cast
    template<typename IndexTy>
    int addEvent(IndexTy typeIndex, const QVariantList& data,
                 EventColorOption colorOption = EventColorOption::Referable,
                 const QVector<EventReference>& references = QVector<EventReference>(),
                 const QVector<EventLocationRemark>& locationRemarks = QVector<EventLocationRemark>()
    ) {
        static_assert (std::is_enum<IndexTy>::value, "IndexTy should be enum");
        return addEvent(static_cast<int>(typeIndex), data, colorOption, references, locationRemarks);
    }

    const Event& getEvent(int index) const {return events.at(index);}
    int size() const {return events.size();}
private:
    QVector<Event> events;
    int numActivelyColoredEvents = 0;
};

#endif // EVENTLOGGING_H
