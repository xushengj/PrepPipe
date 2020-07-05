#include "src/lib/Tree/EventLogging.h"

int EventLogger::addEvent(int typeIndex, const QVector<EventReference>& references, const QVector<EventLocationRemark> &locationRemarks, const QVariantList& data)
{
    int eventIndex = events.size();
    Event e;
    e.eventTypeIndex = typeIndex;
    e.selfIndex = eventIndex;
    e.references = references;
    e.locationRemarks = locationRemarks;
    e.data = data;
    events.push_back(e);
    return eventIndex;
}

