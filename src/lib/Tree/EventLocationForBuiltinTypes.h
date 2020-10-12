#ifndef EVENTLOCATIONFORBUILTINTYPES_H
#define EVENTLOCATIONFORBUILTINTYPES_H

#include <QtGlobal>
#include <QMetaType>
#include <QString>

#include "src/GlobalInclude.h"

/// The location type for plain text. It may either be a "point" or an "interval".
/// If it is a point, then startPos == endPos. Otherwise, startPos < endPos
struct PlainTextLocation {
    indextype startPos = -1;
    indextype endPos = -1;

    bool operator<(const PlainTextLocation& rhs) const {
        if (startPos < rhs.startPos)
            return true;
        if (startPos > rhs.startPos)
            return false;
        return endPos < rhs.endPos;
    }

    bool operator==(const PlainTextLocation& rhs) const {
        return startPos == rhs.startPos && endPos == rhs.endPos;
    }
};
Q_DECLARE_METATYPE(PlainTextLocation)

#endif // EVENTLOCATIONFORBUILTINTYPES_H
