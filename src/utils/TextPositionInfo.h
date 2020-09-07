#ifndef TEXTPOSITIONINFO_H
#define TEXTPOSITIONINFO_H

#include <QMap>

// given a string, compute the row and column number
class TextPositionInfo
{
public:
    TextPositionInfo() = default;
    TextPositionInfo(const QString& text);

    std::pair<int,int> getLineAndColumnNumber(int pos);

private:
    QMap<int, int> lineFeedPosMap;
    int lastLineFeedPos = -1;
};

#endif // TEXTPOSITIONINFO_H
