#include "src/lib/Tree/EventLogging.h"
#include <QDebug>
#include <algorithm>

#define DBG_LOG_EVENTS

EventInterpreter EventLogger::defaultInterp;

int EventLogger::addEvent(int typeIndex, const QVariantList& data, EventColorOption colorOption, const QVector<EventReference>& references, const QVector<EventLocationRemark> &locationRemarks)
{
    int eventIndex = events.size();
    Event e;
    e.eventTypeIndex = typeIndex;
    e.selfIndex = eventIndex;
    if (colorOption == EventColorOption::Active) {
        e.activeColorIndex = numActivelyColoredEvents++;
    } else {
        e.activeColorIndex = -1;
    }
    e.colorOption = colorOption;
    e.references = references;
    std::sort(e.references.begin(), e.references.end());
    e.locationRemarks = locationRemarks;
    e.data = data;
    events.push_back(e);
#ifdef DBG_LOG_EVENTS
    auto dbg = qDebug();
    dbg << "Event" << eventIndex << ":" << typeIndex << ": data={" << data << '}';
    if (colorOption != EventColorOption::Passive) {
        dbg << ", ColorOpt=" << static_cast<int>(colorOption);
    }
    if (!references.isEmpty()) {
        dbg << ", Refs={" << references.front();
        for (int i = 1, n = references.size(); i < n; ++i) {
            dbg << ", " << references.at(i);
        }
        dbg << '}';
    }
    if (!locationRemarks.isEmpty()) {
        dbg << ", Locs={" << locationRemarks.front();
        for (int i = 1, n = locationRemarks.size(); i < n; ++i) {
            dbg << ", " << locationRemarks.at(i);
        }
        dbg << '}';
    }
#endif
    return eventIndex;
}

