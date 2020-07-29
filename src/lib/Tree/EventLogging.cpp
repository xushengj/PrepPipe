#include "src/lib/Tree/EventLogging.h"

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
    e.locationRemarks = locationRemarks;
    e.data = data;
    events.push_back(e);
    return eventIndex;
}

