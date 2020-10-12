#include "src/utils/TextUtilities.h"

TextUtil::TextPositionInfo::TextPositionInfo(const QString& text)
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

std::pair<int,int> TextUtil::TextPositionInfo::getLineAndColumnNumber(int pos) const
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

QString TextUtil::TextPositionInfo::getLocationShortString(const QString& text, int pos) const
{
    int totalLength = text.length();
    if (pos < 0 || pos > totalLength) {
        return tr("<Invalid position: %1>").arg(pos);
    } else if (pos == totalLength) {
        return tr("<End of text>");
    }
    auto p = getLineAndColumnNumber(pos);
    return tr("Line %1, Column %2").arg(QString::number(p.first), QString::number(p.second));
}

QString TextUtil::TextPositionInfo::getLocationString(const QString& text, int startPos, int endPos) const
{
    int totalLength = text.length();
    QString startStr = getLocationShortString(text, startPos);

    // check the start position for now (shared no matter whether it is single-ended or not)
    bool isGetSnippet = true;
    if (startPos < 0 || startPos >= totalLength) {
        isGetSnippet = false;
    }

    // this is just an arbitrary number we picked for now
    const int MAX_SNIPPET_LENGTH = 20;

    if (startPos == endPos) {
        // this is a "point" location instead of an interval
        if (isGetSnippet) {
            // try to get a single-ended text snippet
            int charsLeft = totalLength - startPos;
            QString textSnippet;

            if (charsLeft < MAX_SNIPPET_LENGTH) {
                textSnippet = text.mid(startPos, charsLeft);
            } else {
                textSnippet = text.mid(startPos, MAX_SNIPPET_LENGTH-3);
                textSnippet.append("...");
            }
            return startStr + " \"" + sanitizeSingleLineString(textSnippet) + '\"';
        } else {
            return startStr;
        }
    } else {
        QString endStr = getLocationShortString(text, endPos);
        QString baseStr = tr("[%1] to [%2]").arg(startStr, endStr);
        if (endPos < startPos || endPos > totalLength) {
            isGetSnippet = false;
        }
        if (isGetSnippet) {
            // try to get a double-ended text snippet
            int charsLeft = endPos - startPos;
            QString textSnippet;

            if (charsLeft < MAX_SNIPPET_LENGTH) {
                textSnippet = text.mid(startPos, charsLeft);
            } else {
                int tailDist = (MAX_SNIPPET_LENGTH - 3) / 2;
                int frontDist = (MAX_SNIPPET_LENGTH - 3) - tailDist;
                textSnippet = text.mid(startPos, frontDist) + "..." + text.mid(endPos - tailDist, tailDist);
            }
            return baseStr + " \"" + sanitizeSingleLineString(textSnippet) + '\"';
        } else {
            return baseStr;
        }
    }
}

QString TextUtil::sanitizeSingleLineString(const QString& src)
{
    auto vec = src.toUcs4();
    QString result;
    result.reserve(src.size());
    for (uint c : vec) {
        if (c == '\n') {
            result.append("\\n");
        } else if (c == '\t') {
            result.append("\\t");
        } else if (c == '\r') {
            result.append("\\r");
        } else if (QChar::isPrint(c)) {
            result.append(c);
        } else {
            QString num = QString::number(c, 16);
            int totalLength = (num.length() <= 2? 2 : (num.length() <= 4? 4 : 8));
            int fillLength = totalLength - num.length();
            if (fillLength > 0) {
                num.prepend(QString(fillLength, '0'));
            }
            num.prepend("\\u");
            result.append(num);
        }
    }
    return result;
}
