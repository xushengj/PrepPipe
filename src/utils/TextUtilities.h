#ifndef TEXTPOSITIONINFO_H
#define TEXTPOSITIONINFO_H

#include <QMap>
#include <QCoreApplication>

#include "src/GlobalInclude.h"

namespace TextUtil {

// given a string, compute the row and column number
class TextPositionInfo
{
    Q_DECLARE_TR_FUNCTIONS(TextPositionInfo)
public:
    TextPositionInfo() = default;
    TextPositionInfo(const QString& text);

    std::pair<int,int> getLineAndColumnNumber(int pos) const;


    QString getLocationString(const QString& text, int startPos, int endPos) const;
    QString getLocationShortString(const QString& text, int pos) const;

private:
    QMap<int, int> lineFeedPosMap;
    int lastLineFeedPos = -1;
};

/// The location type for plain text. It may either be a "point" or an "interval".
/// If it is a point, then startPos == endPos. Otherwise, startPos < endPos
/// The location display is handled in src/util/TextPositionInfo.h
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

/// make the string suitable for single line display, for example replacing new line characters with escaped form
QString sanitizeSingleLineString(const QString& src);

} // end namespace TextUtil

Q_DECLARE_METATYPE(TextUtil::PlainTextLocation)



#endif // TEXTPOSITIONINFO_H
