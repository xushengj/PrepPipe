#include "TextPositionInfo.h"

TextPositionInfo::TextPositionInfo(const QString& text)
{
    int idx = 0;
    int pos = 0;
    lastLineFeedPos = -1;
    int curPos = text.indexOf('\n', pos);
    while (curPos != -1) {
        lineFeedPosMap.insert(curPos, idx);
        lastLineFeedPos = curPos;
        pos = curPos + 1;
        idx += 1;
        curPos = text.indexOf('\n', pos);
    }
}

std::pair<int,int> TextPositionInfo::getLineAndColumnNumber(int pos)
{
    // if pos points to an '\n', we consider it as the last character in the last line
    // for example, if the string starts with '\n', then pos with 0 would give row 1, column 1
    int lineNum = lineFeedPosMap.size();
    int lfpos = lastLineFeedPos;
    auto iter = lineFeedPosMap.lowerBound(pos);

    // handle the case where pos is after the last line feed
    if (iter == lineFeedPosMap.end()) {
        Q_ASSERT(pos > lfpos);
        return std::make_pair(lineNum+1, pos-lfpos);
    }

    // handle the case where pos is before / at the first line feed
    if (iter == lineFeedPosMap.begin()) {
        return std::make_pair(1, pos+1);
    }

    // handle the remaining case
    --iter;

    lineNum = iter.value();
    lfpos = iter.key();

    return std::make_pair(lineNum+2, pos-lfpos);
}
