#include "SimpleParser.h"
#include "src/lib/Tree/Tree.h"

// ----------------------------------------------------------------------------
// Parser implementation

QString SimpleParser::getWhiteSpaceRegexPattern(const QStringList& whiteSpaceList)
{
    Q_ASSERT(!whiteSpaceList.isEmpty());
    QString pattern;
    pattern.push_back('(');
    bool isFirst = true;
    for (const auto& str : whiteSpaceList) {
        if (isFirst) {
            isFirst = false;
        } else {
            pattern.push_back('|');
        }
        auto ucs4Vec = str.toUcs4();
        for (const auto& c : ucs4Vec) {
            if (c < 0x80) {
                if (c >= ' ' && c <= '~') {
                    // normal visible characters
                    // escape ones that cause issue in regular expression
                    switch (c) {
                    default: {
                        pattern.push_back(QChar(c));
                    }break;
                    case '\\':
                    case '^':
                    case '$':
                    case '.':
                    case '|':
                    case '?':
                    case '*':
                    case '+':
                    case '(':
                    case ')':
                    case '[':
                    case '{': {
                        pattern.push_back('\\');
                        pattern.push_back(QChar(c));
                    }break;
                    }
                } else {
                    // special characters
                    // print in \u.. unless it matches with a known rule
                    switch (c) {
                    default: {
                        QString num = QString::number(c, 16);
                        int zeroCount = 4 - num.length();
                        pattern.push_back('\\');
                        pattern.push_back('u');
                        for (int i = 0; i < zeroCount; ++i) {
                            pattern.push_back('0');
                        }
                        pattern.push_back(num);
                    }break;
                    case '\t': pattern.push_back(QStringLiteral("\\t")); break;
                    case '\n': pattern.push_back(QStringLiteral("\\n")); break;
                    }
                }
            } else {
                // not in ascii
                // always use \u.... form
                QString num = QString::number(c, 16);
                int length = ((num.length()-1)/2+1)*2;
                if (length < 4) {
                    length = 4;
                }
                int zeroCount = length - num.length();
                pattern.push_back('\\');
                pattern.push_back('u');
                for (int i = 0; i < zeroCount; ++i) {
                    pattern.push_back('0');
                }
                pattern.push_back(num);
            }
        }
    }
    pattern.push_back(')');
    return pattern;
}

SimpleParser::SimpleParser(const Data& d)
    : data(d)
{
    // TODO no handling for generalParanthesis yet

    // use whitespaceList to form regular expression for white space related search
    QString basePattern = getWhiteSpaceRegexPattern(d.whitespaceList);
    regexIndex_SpecialCharacter_OptionalWhiteSpace = state.getRegexIndex(basePattern + '*');
    regexIndex_SpecialCharacter_WhiteSpaces = state.getRegexIndex(basePattern + '+');
    emptyLineRegex = QRegularExpression("^(" + basePattern + "*\n)+");

    // build the boundary name mapping
    for (int i = 0, n = d.namedBoundaries.size(); i < n; ++i) {
        const auto& b = d.namedBoundaries.at(i);
        boundaryNameToIndexMap.insert(b.name, i);
        if (b.ty == BoundaryType::ClassBased) {
            classBasedBoundaryChildList.insert(i, QVector<int>());
        }
    }
    for (auto iter = classBasedBoundaryChildList.begin(), iterEnd = classBasedBoundaryChildList.end(); iter != iterEnd; ++iter) {
        int index = iter.key();
        const auto& b = d.namedBoundaries.at(index);
        for (const auto& p : b.parentList) {
            int parentIndex = boundaryNameToIndexMap.value(p, -1);
            auto parentIter = classBasedBoundaryChildList.find(parentIndex);
            Q_ASSERT(parentIter != classBasedBoundaryChildList.end());
            parentIter.value().push_back(index);
        }
    }

    // build the content type name mapping
    for (int i = 0, n = d.contentTypes.size(); i < n; ++i) {
        const auto& c = d.contentTypes.at(i);
        if (!c.name.isEmpty()) {
            contentTypeNameToIndexMap.insert(c.name, i);
        }
    }

    // build the match rule node name map
    // pseudo root node is the last node
    QHash<QString, int> matchRuleNodeNameToIndexMap;
    for (int i = 0, n = d.matchRuleNodes.size(); i < n; ++i) {
        const auto& rule = d.matchRuleNodes.at(i);
        matchRuleNodeNameToIndexMap.insert(rule.name, i);
    }
    childNodeMatchRules.resize(d.matchRuleNodes.size()+1);
    auto populateChildNodeMatchRules = [&](int matchRuleNodeIndex, const QVector<int>& childNodes) -> void {
        // the pattern matching when a set of nodes are activated is done as follows:
        // all patterns have a "pass" value, and all patterns from all nodes with the same "pass" is considered together
        // the matching start from pass 0, then 1, 2, ..., and then -inf, .., -2, -1
        QMap<int, QHash<int, QVector<int>>> positivePasses;
        QMap<int, QHash<int, QVector<int>>> negativePasses;
        for (int nodeIndex : childNodes) {
            const auto& nodeData = d.matchRuleNodes.at(nodeIndex);
            for (int patternIndex = 0; patternIndex < nodeData.patterns.size(); ++patternIndex) {
                const auto& pattern = nodeData.patterns.at(patternIndex);
                int passIndex = pattern.patternPass;
                if (passIndex >= 0) {
                    positivePasses[passIndex][nodeIndex].push_back(patternIndex);
                } else {
                    negativePasses[passIndex][nodeIndex].push_back(patternIndex);
                }
            }
        }
        auto& rulesData = childNodeMatchRules[matchRuleNodeIndex];
        for (auto iter = positivePasses.begin(), iterEnd = positivePasses.end(); iter != iterEnd; ++iter) {
            rulesData.push_back(SimpleParser::MatchPassData());
            auto& passData = rulesData.back();
            passData.pass = iter.key();
            passData.patterns = iter.value();
        }
        for (auto iter = negativePasses.begin(), iterEnd = negativePasses.end(); iter != iterEnd; ++iter) {
            rulesData.push_back(SimpleParser::MatchPassData());
            auto& passData = rulesData.back();
            passData.pass = iter.key();
            passData.patterns = iter.value();
        }
    };

    for (int i = 0, n = d.matchRuleNodes.size(); i < n; ++i) {
        const auto& rule = d.matchRuleNodes.at(i);
        QVector<int> childNodesVec;
        for (const auto& child : rule.childNodeNameList) {
            int childIndex = matchRuleNodeNameToIndexMap.value(child, -1);
            Q_ASSERT(childIndex != -1);
            childNodesVec.push_back(childIndex);
        }
        populateChildNodeMatchRules(i, childNodesVec);
    }
    {
        QVector<int> topNodes;
        for (const QString& topNode : d.topNodeList) {
            int topNodeIndex = matchRuleNodeNameToIndexMap.value(topNode, -1);
            Q_ASSERT(topNodeIndex != -1);
            topNodes.push_back(topNodeIndex);
        }
        populateChildNodeMatchRules(d.matchRuleNodes.size(), topNodes);
    }
}

bool SimpleParser::performParsing(const QString& src, Tree& dest, EventLogger *logger)
{
    state.clear();
    builder.clear();

    state.set(src, logger);

    struct RuleStackFrame {
        TreeBuilder::Node* ptr = nullptr;
        SimpleParser::ContextMatchRuleData childMatchRules;
        int event = -1;
        QVector<std::pair<int,int>> passData; // pass -> [<frame, matchPassIndex>]

        RuleStackFrame() = default;
        RuleStackFrame(TreeBuilder::Node* node, const SimpleParser::ContextMatchRuleData& child, int eventID)
            : ptr(node), childMatchRules(child), event(eventID)
        {}
        RuleStackFrame(const RuleStackFrame&) = default;
        RuleStackFrame(RuleStackFrame&&) = default;
    };
    QVector<RuleStackFrame> ruleStack;

    int pos = 0;
    int lastEventChangingPos = -1;
    int lastEventChangingFrame = -1;
    int length = src.length();
    QHash<int, int> treeNodeSequenceNumberToEventMap; // [sequence number] -> <node add event id>

    auto skipEmptyLines = [&]() -> void {
        int startPos = pos;
        QStringRef str = src.midRef(pos);
        auto match = emptyLineRegex.match(str, 0, QRegularExpression::NormalMatch, QRegularExpression::AnchoredMatchOption);
        if (match.hasMatch()) {
            int matchStart = match.capturedStart();
            int matchEnd = match.capturedEnd();
            Q_ASSERT(matchStart == 0);
            int dist = matchEnd;
            pos += dist;
            state.curPosition = pos;
            auto startPosPair = state.posInfo.getLineAndColumnNumber(startPos);
            auto endPosPair = state.posInfo.getLineAndColumnNumber(pos-1);
            lastEventChangingPos = SimpleParserEvent::Log(SimpleParserEvent::EmptyLineSkipped, logger, startPos, pos, startPosPair.first, endPosPair.first);
        }
    };

    auto pushFrame = [&](TreeBuilder::Node* node, const SimpleParser::ContextMatchRuleData& child, int eventID) -> void {
        // step 1: push the frame
        ruleStack.push_back(RuleStackFrame(node, child, eventID));

        // step 2: figure out ordering of passes across all frames on the stack
        struct TmpPassRecord {
            int passVal;
            int frame;
            int framePassIndex;
        };
        int numPassData = 0;
        for (int frameIndex = 0, numFrame = ruleStack.size(); frameIndex < numFrame; ++frameIndex) {
            const auto& frame = ruleStack.at(frameIndex);
            numPassData += frame.childMatchRules.size();
        }
        std::vector<std::pair<int, std::pair<int, int>>> passRecords; // <passVal, <frame, framePassIndex>>
        passRecords.reserve(numPassData);

        for (int frameIndex = 0, numFrame = ruleStack.size(); frameIndex < numFrame; ++frameIndex) {
            const auto& frame = ruleStack.at(frameIndex);
            for (int passDataIndex = 0, numPass = frame.childMatchRules.size(); passDataIndex < numPass; ++passDataIndex) {
                const auto& passData = frame.childMatchRules.at(passDataIndex);
                int passVal = passData.pass;
                passRecords.push_back(std::make_pair(passVal, std::make_pair(frameIndex, passDataIndex)));
            }
        }

        std::sort(passRecords.begin(), passRecords.end(),
        [](
                  const std::pair<int, std::pair<int, int>>& lhs,
                  const std::pair<int, std::pair<int, int>>& rhs) -> bool {
            if (lhs.first != rhs.first) {
                // pass is different
               if (lhs.first >= 0 && rhs.first < 0) {
                    return true;
                } else if (lhs.first < 0 && rhs.first >= 0) {
                    return false;
                } else {
                    // same interval
                    return lhs.first < rhs.first;
                }
            } else {
                // same pass
                return lhs.second.first < rhs.second.first;
            }
        });

        auto& passData = ruleStack.back().passData;
        passData.reserve(passRecords.size());
        for (const auto& record : passRecords) {
            passData.push_back(record.second);
        }
    };

    // bootstrap the first node
    {
        int rootNodeEventID = SimpleParserEvent::Log(SimpleParserEvent::RootNodeCreation, logger);
        auto* rootPtr = builder.addNode(nullptr);
        pushFrame(rootPtr, childNodeMatchRules.back(), rootNodeEventID);
        lastEventChangingFrame = rootNodeEventID;
        treeNodeSequenceNumberToEventMap.insert(rootPtr->getSequenceNumber(), rootNodeEventID);
    }

    // main loop
    bool isMatchFailed = false;
    while (!ruleStack.isEmpty() && (pos < length)) {
        if (data.flag_skipEmptyLineBeforeMatching) {
            skipEmptyLines();
        }

        if (pos >= length) {
            break;
        }

        const auto& frame = ruleStack.back();
        const auto passData = frame.passData; // implicit sharing; create another reference so that no bad access is made when frame is poped inside the body
        QList<int> passMatchFailEvents;
        int startPos = pos;
        bool isMatchFound = false;
        for (const auto& pass : passData) {
            int frameIndex = pass.first;
            int passIndex = pass.second;
            const auto& curPassData = ruleStack.at(frameIndex).childMatchRules.at(passIndex);
            int passNum = curPassData.pass;

            QList<int> positiveMatchEvents;
            QList<int> negativeMatchEvents;
            PatternMatchResult curBestResult;
            int bestResultRuleNodeIndex = -1;
            int bestResultPatternIndex = -1;

            for(auto iter = curPassData.patterns.begin(), iterEnd = curPassData.patterns.end(); iter != iterEnd; ++iter) {
                int matchRuleNodeIndex = iter.key();
                const auto& patterns = data.matchRuleNodes.at(matchRuleNodeIndex).patterns;
                for (int patternIndex : iter.value()) {
                    const auto& curPattern = patterns.at(patternIndex);
                    PatternMatchResult curResult = tryPattern(curPattern, pos, frame.event);
                    if (logger) {
                        Q_ASSERT(curResult.nodeFinalEvent >= 0);
                    }
                    if (!curResult) {
                        negativeMatchEvents.push_back(curResult.nodeFinalEvent);
                        continue;
                    }
                    positiveMatchEvents.push_back(curResult.nodeFinalEvent);
                    if (!curBestResult || curResult.isBetterThan(curBestResult)) {
                        // first match
                        curBestResult = curResult;
                        bestResultRuleNodeIndex = matchRuleNodeIndex;
                        bestResultPatternIndex = patternIndex;
                    }
                }
            }

            if (!curBestResult) {
                int matchFailEvent = SimpleParserEvent::Log(SimpleParserEvent::MatchPassNoMatching, logger, frame.event, passNum, frameIndex, negativeMatchEvents);
                if (matchFailEvent >= 0) {
                    passMatchFailEvents.push_back(matchFailEvent);
                }
                continue;
            } else {
                // we get a match
                isMatchFound = true;

                TreeBuilder::Node* node = curBestResult.node;
                Q_ASSERT(node);

                pos += curBestResult.totalConsumedLength;
                state.curPosition = pos;
                lastEventChangingPos = curBestResult.nodeFinalEvent;

                // merge the pass fail events into negative match events
                negativeMatchEvents.append(passMatchFailEvents);

                // if the best match is an early exit pattern, we also go to its parent
                auto& bestMatchFrame = ruleStack[frameIndex];
                if (curBestResult.node->typeName.isEmpty()) {
                    lastEventChangingFrame = SimpleParserEvent::Log(SimpleParserEvent::FramePopedForEarlyExit, logger, bestMatchFrame.event, curBestResult.nodeFinalEvent, positiveMatchEvents, negativeMatchEvents);
                    assert(frameIndex > 0);
                    ruleStack.resize(frameIndex);
                    break; // stop trying other passes
                }

                TreeBuilder::Node* parent = bestMatchFrame.ptr;
                node->setParent(parent);

                int nodeAddEvent = SimpleParserEvent::Log(SimpleParserEvent::NodeAdded, logger, node->typeName, frame.event, curBestResult.nodeFinalEvent, positiveMatchEvents, negativeMatchEvents, startPos, pos);
                treeNodeSequenceNumberToEventMap.insert(node->getSequenceNumber(), nodeAddEvent);

                if (frameIndex != ruleStack.size() - 1) {
                    // we are not matching at the top most frame
                    // just drop all frames above the best matching frame
                    ruleStack.resize(frameIndex + 1);
                    lastEventChangingFrame = nodeAddEvent;
                }


                const auto& childRules = childNodeMatchRules.at(bestResultRuleNodeIndex);
                if (!childRules.isEmpty()) {
                    pushFrame(node, childRules, nodeAddEvent);
                    lastEventChangingFrame = nodeAddEvent;
                }

                break; // stop trying other passes
            }
        }
        if (!isMatchFound) {
            // a match is not made
            lastEventChangingFrame = SimpleParserEvent::Log(SimpleParserEvent::MatchFailed, logger, frame.event, passMatchFailEvents);
            isMatchFailed = true;
            break;
        }
    }

    int finishEvent = SimpleParserEvent::Log(SimpleParserEvent::MatchFinished, logger, lastEventChangingFrame, pos);
    QVector<int> sequenceNumberTable;
    Tree tree(builder, sequenceNumberTable);
    dest.swap(tree);

    for (int i = 0, n = sequenceNumberTable.size(); i < n; ++i) {
        int seqNum = sequenceNumberTable.at(i);
        auto iter = treeNodeSequenceNumberToEventMap.find(seqNum);
        Q_ASSERT(iter != treeNodeSequenceNumberToEventMap.end());
        SimpleParserEvent::Log(SimpleParserEvent::TextToNodePositionMapping, logger, i, iter.value());
    }

    if (isMatchFailed) {
        if (logger) {
            logger->passFailed(lastEventChangingFrame);
        }
        return false;
    }

    // check if all text is consumed
    while (pos < src.length()) {
        QStringRef str(&src, pos, src.length() - pos);
        bool isWhiteSpaceFound = false;
        if (str.startsWith('\n')) {
            pos += 1;
            continue;
        }
        for (const auto& ws : data.whitespaceList) {
            if (str.startsWith(ws)) {
                pos += ws.length();
                isWhiteSpaceFound = true;
                break;
            }
        }
        if (isWhiteSpaceFound)
            continue;

        // we find strings that are not "whitespace"
        SimpleParserEvent::Log(SimpleParserEvent::GarbageAtEnd, logger, str.position(), finishEvent);
        return false;
    }

    state.clear();
    builder.clear();

    if (logger) {
        logger->passSucceeded();
    }
    return true;
}

void SimpleParser::ParseState::clear()
{
    stringLiteralPositionMap.clear();
    for (auto& regexPosMap : regexMatchPositionMap) {
        regexPosMap.clear();
    }
    str = nullptr;
    logger = nullptr;
    posInfo = TextUtil::TextPositionInfo();

    // the following two is persistent across uses
    // regexPatternToIndexMap.clear();
    // regexList.clear();
}

void SimpleParser::ParseState::set(const QString& text, EventLogger* loggerArg)
{
    str = &text;
    logger = loggerArg;
    strLength = text.length();
    curPosition = 0;
    posInfo = TextUtil::TextPositionInfo(text);
}

int SimpleParser::ParseState::getRegexIndex(const QString& pattern)
{
    int regexIndex = regexPatternToIndexMap.value(pattern, -1);
    if (regexIndex == -1) {
        regexIndex = regexList.size();
        QRegularExpression regex(pattern, QRegularExpression::MultilineOption | QRegularExpression::DontCaptureOption | QRegularExpression::UseUnicodePropertiesOption);
        Q_ASSERT(regex.isValid());
        regexList.push_back(regex);
        regexPatternToIndexMap.insert(pattern, regexIndex);
        regexMatchPositionMap.push_back(QMap<int, std::pair<int, ParseState::RegexMatchData>>());
    }
    return regexIndex;
}

SimpleParser::PatternMatchResult SimpleParser::tryPattern(const Pattern& pattern, int position, int patternTestSourceEvent)
{
    // helper function
    auto setDeclByPatternElement = [](BoundaryDeclaration& decl, const PatternElement& pe) -> void {
        decl.str = pe.str;
        switch (pe.ty) {
        case PatternElement::ElementType::Content: {
            qFatal("Content type pattern element enters anonymous boundary handling code");
        }break;
        case PatternElement::ElementType::AnonymousBoundary_StringLiteral: {
            decl.decl = BoundaryDeclaration::DeclarationType::Value;
            decl.ty = BoundaryType::StringLiteral;
        }break;
        case PatternElement::ElementType::AnonymousBoundary_Regex: {
            decl.decl = BoundaryDeclaration::DeclarationType::Value;
            decl.ty = BoundaryType::Regex;
        }break;
        case PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace: {
            decl.decl = BoundaryDeclaration::DeclarationType::Value;
            decl.ty = BoundaryType::SpecialCharacter_OptionalWhiteSpace;
        }break;
        case PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_WhiteSpaces: {
            decl.decl = BoundaryDeclaration::DeclarationType::Value;
            decl.ty = BoundaryType::SpecialCharacter_WhiteSpaces;
        }break;
        case PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_LineFeed: {
            decl.decl = BoundaryDeclaration::DeclarationType::Value;
            decl.ty = BoundaryType::SpecialCharacter_LineFeed;
        }break;
        case PatternElement::ElementType::NamedBoundary: {
            decl.decl = BoundaryDeclaration::DeclarationType::NameReference;
            decl.ty = BoundaryType::StringLiteral; // won't be used anyway
        }break;
        }
    };
    SimpleParser::PatternMatchResult result;
    QStringList keyList;
    QVector<std::pair<int,int>> valueList; // <start position, length>
    int elementIndex = 0;
    int elementCount = pattern.pattern.size();
    int totalConsumeCount = 0;
    int boundaryConsumeCount = 0;
    while (elementIndex < elementCount) {
        const PatternElement& pe = pattern.pattern.at(elementIndex);
        BoundaryDeclaration decl;
        int nextElementIndex = elementIndex+1;
        int curTestingElementIndex = elementIndex;
        int curWSElementIndex = -1;
        int contentIndex = -1;
        QString contentExportName;
        QString boundaryExportName;
        QString wsAfterContentExportName;
        bool chopWSAfterContent = false;;
        bool mandatoryWSAfterContent = false;
        if (pe.ty == PatternElement::ElementType::Content) {
            contentExportName = pe.elementName;
            Q_ASSERT(nextElementIndex < elementCount);
            if (pe.str.isEmpty()) {
                contentIndex = 0;
            } else {
                contentIndex = contentTypeNameToIndexMap.value(pe.str, -1);
                Q_ASSERT(contentIndex != -1);
            }
            const PatternElement& nextPE = pattern.pattern.at(nextElementIndex);
            curTestingElementIndex = nextElementIndex;
            curWSElementIndex = nextElementIndex;
            nextElementIndex += 1;
            Q_ASSERT(nextPE.ty != PatternElement::ElementType::Content);

            // set to true if the exportnames and boundary declaration is specially handled
            bool isWSScenaro = false;

            if ((nextPE.ty == PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace
              || nextPE.ty == PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_WhiteSpaces)
                    && nextElementIndex < elementCount) {
                // the next element is a white space, which should be checked AFTER its next element is found
                const PatternElement& nextNextPE = pattern.pattern.at(nextElementIndex);
                bool isFallback = false;
                switch (nextNextPE.ty) {
                default:
                    isFallback = false;
                    break;
                case PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace:
                case PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_WhiteSpaces:
                case PatternElement::ElementType::Content:
                    isFallback = true;
                    break;
                }
                if (!isFallback) {
                    // we can check the next pattern
                    isWSScenaro = true;
                    curTestingElementIndex = nextElementIndex;
                    nextElementIndex += 1;
                    boundaryExportName = nextNextPE.elementName;
                    setDeclByPatternElement(decl, nextNextPE);
                    wsAfterContentExportName = nextPE.elementName;
                    chopWSAfterContent = true;
                    mandatoryWSAfterContent = (nextPE.ty == PatternElement::ElementType::AnonymousBoundary_SpecialCharacter_WhiteSpaces);
                }
                // do nothing if we really cannot find a better way of matching the whitespace
            }
            if (!isWSScenaro) {
                boundaryExportName = nextPE.elementName;
                setDeclByPatternElement(decl, nextPE);
            }
        } else {
            // this is a boundary
            boundaryExportName = pe.elementName;
            setDeclByPatternElement(decl, pe);
        }

        int currentPosition = position + totalConsumeCount;
        std::pair<int, int> pos = findBoundary(currentPosition, decl, contentIndex, chopWSAfterContent);
        if (pos.first < 0) {
            // no matches
            result.nodeFinalEvent = SimpleParserEvent::Log(SimpleParserEvent::PatternNotMatched, state.logger,
                                                           position, currentPosition,
                                                           patternTestSourceEvent,
                                                           curTestingElementIndex, pattern.pattern.at(curTestingElementIndex),
                                                           QList<int>()); // TODO add tracking of matching events
            return result;
        }

        // okay, we have a match
        totalConsumeCount += pos.first + pos.second;
        boundaryConsumeCount += pos.second;

        if (contentIndex == -1) {
            Q_ASSERT(pos.first == 0);
        } else {
            int contentStart = currentPosition;
            int contentEnd = pos.first; // this is a length instead of absolute pos
            int wsEnd = pos.first;
            if (chopWSAfterContent) {
                int tailChopLen = getWhitespaceTailChopLength(currentPosition, pos.first);
               contentEnd -= tailChopLen;
               boundaryConsumeCount += tailChopLen;
               if (tailChopLen == 0 && mandatoryWSAfterContent) {
                   // no matches
                   result.nodeFinalEvent = SimpleParserEvent::Log(SimpleParserEvent::PatternNotMatched, state.logger,
                                                                  currentPosition, currentPosition + pos.first,
                                                                  patternTestSourceEvent,
                                                                  curWSElementIndex, pattern.pattern.at(curWSElementIndex),
                                                                  QList<int>()); // TODO add tracking of matching events
                   // curWSElementIndex
                   return result;
               }
            }
            if (!contentExportName.isEmpty()) {
                keyList.push_back(contentExportName);
                valueList.push_back(std::make_pair(contentStart, contentEnd));
            }
            if (!wsAfterContentExportName.isEmpty()) {
                keyList.push_back(wsAfterContentExportName);
                valueList.push_back(std::make_pair(contentEnd, wsEnd));
            }
        }
        if (!boundaryExportName.isEmpty()) {
            keyList.push_back(boundaryExportName);
            valueList.push_back(std::make_pair(currentPosition + pos.first, pos.second));
        }

        elementIndex = nextElementIndex;
    }

    // all elements are matched
    TreeBuilder::Node* node = builder.allocateNode();
    node->typeName = pattern.typeName;
    node->keyList = keyList;
    node->valueList.reserve(valueList.size());
    for (int i = 0, n = valueList.size(); i < n; ++i) {
        const auto& valuePos = valueList.at(i);
        node->valueList.push_back(state.str->mid(valuePos.first, valuePos.second));
    }
    result.node = node;
    result.totalConsumedLength = totalConsumeCount;
    result.boundaryConsumedLength = boundaryConsumeCount;
    // TODO currently we don't track element matching events yet
    result.nodeFinalEvent = SimpleParserEvent::Log(SimpleParserEvent::PatternMatched, state.logger, position, position + totalConsumeCount, patternTestSourceEvent, QList<int>());
    return result;
}

int SimpleParser::getWhitespaceTailChopLength(int startPos, int length)
{
    int totalLength = 0;
    QStringRef strRef = state.str->midRef(startPos, length);
    bool isChanged = true;
    while (isChanged) {
        isChanged = false;
        for (const auto& ws : data.whitespaceList) {
            if (strRef.endsWith(ws)) {
                int len = ws.length();
                totalLength += len;
                strRef.chop(len);
                isChanged = true;
            }
        }
    }
    return totalLength;
}

std::pair<int, int> SimpleParser::findBoundary(int pos, const BoundaryDeclaration& decl, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    switch (decl.decl) {
    case BoundaryDeclaration::DeclarationType::Value: {
        switch (decl.ty) {
        case BoundaryType::StringLiteral:   return findBoundary_StringLiteral   (pos, decl.str, precedingContentTypeIndex, chopWSAfterContent);
        case BoundaryType::Regex:           return findBoundary_Regex           (pos, decl.str, precedingContentTypeIndex, chopWSAfterContent);

        case BoundaryType::SpecialCharacter_OptionalWhiteSpace: return findBoundary_SpecialCharacter_OptionalWhiteSpace (pos, precedingContentTypeIndex, chopWSAfterContent);
        case BoundaryType::SpecialCharacter_WhiteSpaces:        return findBoundary_SpecialCharacter_WhiteSpaces        (pos, precedingContentTypeIndex, chopWSAfterContent);
        case BoundaryType::SpecialCharacter_LineFeed:           return findBoundary_SpecialCharacter_LineFeed           (pos, precedingContentTypeIndex, chopWSAfterContent);

        default: {
            qFatal("Unhandled value based boundary");
        }break;
        }
    }break;
    case BoundaryDeclaration::DeclarationType::NameReference: {
        int index = boundaryNameToIndexMap.value(decl.str, -1);
        const NamedBoundary& b = data.namedBoundaries.at(index);
        switch (b.ty) {
        case BoundaryType::Concatenation: {
            if (b.elements.isEmpty()) {
                return std::make_pair(-1, 0);
            }
            std::pair<int, int> firstResult = findBoundary(pos, b.elements.front(), precedingContentTypeIndex, chopWSAfterContent);
            if (firstResult.first < 0) {
                return std::make_pair(-1, 0);
            }
            int consumeCount = firstResult.first + firstResult.second;
            for (int i = 1, n = b.elements.size(); i < n; ++i) {
                std::pair<int, int> result = findBoundary(pos + consumeCount, b.elements.at(i), -1, chopWSAfterContent);
                Q_ASSERT(result.first <= 0);
                if (result.first == 0) {
                    consumeCount += result.second;
                } else {
                    return std::make_pair(-1, 0);
                }
            }
            return std::make_pair(firstResult.first, consumeCount - firstResult.first);
        }break;
        case BoundaryType::ClassBased: return findBoundary_ClassBased(pos, index, precedingContentTypeIndex, chopWSAfterContent);
        default: {
            qFatal("Unhandled named boundary");
        }break;
        }
    }
    }
    Q_UNREACHABLE();
}

std::pair<int, int> SimpleParser::findBoundary_ClassBased(int pos, int boundaryIndex, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    const NamedBoundary& boundary = data.namedBoundaries.at(boundaryIndex);
    Q_ASSERT(boundary.ty == BoundaryType::ClassBased);
    std::pair<int, int> bestResult(-1, 0);
    for (const auto& decl : boundary.elements) {
        std::pair<int, int> curResult = findBoundary(pos, decl, precedingContentTypeIndex, chopWSAfterContent);
        if (curResult.first < 0)
            continue;
        if ((bestResult.first == -1) || (bestResult.first > curResult.first) || (bestResult.first == curResult.first && bestResult.second < curResult.second)) {
            bestResult = curResult;
        }
    }
    auto iter = classBasedBoundaryChildList.find(boundaryIndex);
    Q_ASSERT(iter != classBasedBoundaryChildList.end());
    for (int child : iter.value()) {
        std::pair<int, int> curResult = findBoundary_ClassBased(pos, child, precedingContentTypeIndex, chopWSAfterContent);
        if (curResult.first < 0)
            continue;
        if ((bestResult.first == -1) || (bestResult.first > curResult.first) || (bestResult.first == curResult.first && bestResult.second < curResult.second)) {
            bestResult = curResult;
        }
    }

    return bestResult;
}

int SimpleParser::contentCheck(const ContentType& content, int startPos, int length, bool chopWSAfterContent)
{
    int actualLength = length;
    if (actualLength > 0 && chopWSAfterContent) {
        actualLength -= getWhitespaceTailChopLength(startPos, startPos + length);
    }
    // TODO
    Q_UNUSED(content)
    Q_UNUSED(startPos)
    Q_UNUSED(actualLength)
    return 0;
}

int SimpleParser::findNextStringMatch(int startPos, const QString& str)
{
    QMap<int, int>& map = state.stringLiteralPositionMap[str];

    // remove "expired" entries first
    {
        auto iter = map.begin();
        while (iter != map.end()) {
            if (iter.key() < state.curPosition) {
                iter = map.erase(iter);
            } else {
                break;
            }
        }
    }

    // check if there is already a record that covers this search
    auto iter = map.lowerBound(startPos);
    if (iter != map.end()) {
        Q_ASSERT(iter.key() >= startPos);
        if (iter.key() == startPos || (iter.key() > startPos && iter.value() <= startPos)) {
            if (iter.key() >= state.strLength) {
                // this mean that the string do not appear in the rest of text
                return -1;
            }
            // this is a valid result
            return iter.key() - startPos;
        }
    }


    // actually do the search
    int index = state.str->indexOf(str, startPos);
    int key = index;
    if (index == -1) {
        key = state.strLength;
    }
    if (iter != map.end() && key == iter.key()) {
        iter.value() = startPos;
    } else {
        map.insert(key, startPos);
    }

    if (index == -1) {
        return -1;
    }
    return index - startPos;
}

std::pair<int, int> SimpleParser::findNextRegexMatch(int startPos, const QRegularExpression& regex, QMap<int, std::pair<int, ParseState::RegexMatchData>>& positionMap)
{
    // remove "expired" entries first
    {
        auto iter = positionMap.begin();
        while (iter != positionMap.end()) {
            if (iter.key() < state.curPosition) {
                iter = positionMap.erase(iter);
            } else {
                break;
            }
        }
    }

    // check if there is already a record that covers this search
    auto iter = positionMap.lowerBound(startPos);
    if (iter != positionMap.end()) {
        Q_ASSERT(iter.key() >= startPos);
        if (iter.key() == startPos || (iter.key() > startPos && iter.value().first <= startPos)) {
            if (iter.key() >= state.strLength) {
                // this mean that the string do not appear in the rest of text
                return std::make_pair(-1, 0);
            }
            // this is a valid result
            return std::make_pair(iter.key() - startPos, iter.value().second.length);
        }
    }

    // actually do the match
    auto match = regex.match(*state.str, startPos);
    if (match.hasMatch()) {
        int startOffset = match.capturedStart();
        int length = match.capturedEnd() - startOffset;
        int matchPos = startOffset + startPos;

        if (iter != positionMap.end() && iter.key() == matchPos) {
            // same match
            iter.value().first = startPos;
        } else {
            // new match
            ParseState::RegexMatchData data;
            data.length = length;
            int numCaptures = regex.captureCount();
            data.namedCaptures.reserve(numCaptures);
            for (int i = 0; i < numCaptures; ++i) {
                data.namedCaptures.push_back(match.capturedRef(i+1));
            }
            positionMap.insert(matchPos, std::make_pair(startPos, data));
        }

        return std::make_pair(startOffset - startPos, length);
    } else {
        if (iter != positionMap.end() && iter.key() == state.strLength) {
            iter.value().first = startPos;
        } else {
            positionMap.insert(state.strLength, std::make_pair(startPos, ParseState::RegexMatchData()));
        }
        return std::make_pair(-1, 0);
    }
    Q_UNREACHABLE();
}

std::pair<int, int> SimpleParser::findBoundary_StringLiteral(int pos, const QString& str, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    if (precedingContentTypeIndex == -1) {
        if (state.str->midRef(pos).startsWith(str)) {
            return std::make_pair(0, str.length());
        }
        return std::make_pair(-1, 0);
    }

    const ContentType& c = data.contentTypes.at(precedingContentTypeIndex);
    int curPos = pos;
    while (true) {
        int dist = findNextStringMatch(curPos, str);
        if (dist == -1) {
            return std::make_pair(-1, 0);
        }
        int result = contentCheck(c, curPos, dist, chopWSAfterContent);
        if (result < 0) {
            return std::make_pair(-1, 0);
        } else if (result == 0) {
            // done
            return std::make_pair(curPos + dist - pos, str.length());
        } else {
            Q_ASSERT(result > dist);
            curPos += result;
        }
    }
}

std::pair<int, int> SimpleParser::findBoundary_Regex(int pos, const QString& str, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    return findBoundary_Regex_Impl(pos, state.getRegexIndex(str), precedingContentTypeIndex, chopWSAfterContent);
}

std::pair<int, int> SimpleParser::findBoundary_SpecialCharacter_OptionalWhiteSpace(int pos, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    // it seems that if the next character is \n, it will give "no match" result
    // add workaround here
    std::pair<int, int> result = findBoundary_Regex_Impl(pos, regexIndex_SpecialCharacter_OptionalWhiteSpace, precedingContentTypeIndex, chopWSAfterContent);
    if (result.first == -1) {
        return std::make_pair(0, 0);
    }
    return result;
}

std::pair<int, int> SimpleParser::findBoundary_SpecialCharacter_WhiteSpaces(int pos, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    return findBoundary_Regex_Impl(pos, regexIndex_SpecialCharacter_WhiteSpaces, precedingContentTypeIndex, chopWSAfterContent);
}

std::pair<int, int> SimpleParser::findBoundary_SpecialCharacter_LineFeed(int pos, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    return findBoundary_StringLiteral(pos, QStringLiteral("\n"), precedingContentTypeIndex, chopWSAfterContent);
}

std::pair<int, int> SimpleParser::findBoundary_Regex_Impl(int pos, int regexIndex, int precedingContentTypeIndex, bool chopWSAfterContent)
{
    const QRegularExpression& regex = state.regexList.at(regexIndex);
    auto& positionMap = state.regexMatchPositionMap[regexIndex];
    std::pair<int,int> dists = findNextRegexMatch(pos, regex, positionMap);
    if (precedingContentTypeIndex == -1) {
        if (dists.first != 0) {
            return std::make_pair(-1, 0);
        } else {
            return dists;
        }
    }

    const ContentType& c = data.contentTypes.at(precedingContentTypeIndex);
    int firstResult = contentCheck(c, pos, dists.first, chopWSAfterContent);
    if (firstResult < 0) {
        return std::make_pair(-1, 0);
    } else if (firstResult == 0) {
        return dists;
    }

    // need to loop
    int curPos = pos + firstResult;
    while (true) {
        std::pair<int,int> curDist = findNextRegexMatch(curPos, regex, positionMap);
        if (curDist.first == -1) {
            return std::make_pair(-1, 0);
        }
        int result = contentCheck(c, curPos, curDist.second, chopWSAfterContent);
        if (result < 0) {
            return std::make_pair(-1, 0);
        } else if (result == 0) {
            // done
            return std::make_pair(curPos + curDist.first - pos, curDist.second);
        } else {
            curPos += result;
        }
    }
}

// ----------------------------------------------------------------------------

bool SimpleParser::Data::validate(QString& err) const
{
    // TODO
    Q_UNUSED(err)
    return true;
}

// ----------------------------------------------------------------------------
// XML input / output

namespace {
const QString XML_MATCH_RULE_NODE_LIST = QStringLiteral("MatchRuleNodes");
const QString XML_MATCH_RULE_NODE = QStringLiteral("Node");
const QString XML_NAMED_BOUNDARY_LIST = QStringLiteral("NamedBoundaries");
const QString XML_NAMED_BOUNDARY = QStringLiteral("NamedBoundary");
const QString XML_CONTENT_TYPE_LIST = QStringLiteral("ContentTypes");
const QString XML_CONTENT_TYPE = QStringLiteral("ContentType");
const QString XML_BALANCED_PARENTHESIS_LIST = QStringLiteral("BalancedParanthesisList");
const QString XML_PARENTHESIS = QStringLiteral("Paranthesis");
const QString XML_WHITESPACE_LIST = QStringLiteral("WhiteSpaces");
const QString XML_WHITESPACE = QStringLiteral("WhiteSpace");
const QString XML_TOP_NODE_LIST = QStringLiteral("TopMatchRuleNodeList");
const QString XML_NODE = QStringLiteral("Node");

const QString XML_TYPE = QStringLiteral("Type");
const QString XML_NAME = QStringLiteral("Name");
const QString XML_CHILD_LIST = QStringLiteral("Children");
const QString XML_CHILD = QStringLiteral("Child");
const QString XML_PARENT_LIST = QStringLiteral("Parents");
const QString XML_PARENT = QStringLiteral("Parent");
const QString XML_PATTERN_LIST = QStringLiteral("Patterns");
const QString XML_PATTERN = QStringLiteral("Pattern");

const QString XML_TYPENAME = QStringLiteral("TypeName");
const QString XML_PASS = QStringLiteral("Pass");
const QString XML_PATTERN_ELEMENT_LIST = QStringLiteral("PatternElements");
const QString XML_PATTERN_ELEMENT = QStringLiteral("Element");

const QString XML_STRINGLITERAL = QStringLiteral("StringLiteral");
const QString XML_STRING = QStringLiteral("String");
const QString XML_REGEX = QStringLiteral("Regex");
const QString XML_CONTENT = QStringLiteral("Content");
const QString XML_PATTERN_ELEMENT_TYPE = QStringLiteral("ElementType");
const QString XML_PATTERN_ELEMENT_EXPORT_NAME = QStringLiteral("ExportName");

const QString XML_SPECIAL_CHARACTER_WHITESPACE_OPTIONAL = QStringLiteral("WhiteSpace_Optional");
const QString XML_SPECIAL_CHARACTER_WHITESPACES = QStringLiteral("WhiteSpaces");
const QString XML_SPECIAL_CHARACTER_LINEFEED = QStringLiteral("LineFeed");
const QString XML_CONCATENATION = QStringLiteral("Concatenation");
const QString XML_CLASSBASED = QStringLiteral("ClassBased");
const QString XML_ELEMENT_LIST = QStringLiteral("Elements");
const QString XML_ELEMENT = QStringLiteral("Element");

const QString XML_DECLARATION_TYPE = QStringLiteral("DeclarationType");
const QString XML_VALUE = QStringLiteral("Value");
const QString XML_NAME_REFERENCE = QStringLiteral("NameReference");

const QString XML_CONTENT_ACCEPTED_PARENTHESIS_LIST = QStringLiteral("AcceptedParenthesis");

const QString XML_OPEN = QStringLiteral("Open");
const QString XML_CLOSE = QStringLiteral("Close");

const QString XML_FLAGS = QStringLiteral("Flags");
const QString XML_FLAG_SKIP_EMPTY_LINE_BEFORE_MATCHING = QStringLiteral("SkipEmptyLineBeforeMatching");

template<typename ElTy>
void writeSortedVec(QXmlStreamWriter& xml, const QVector<ElTy>& vec, const QString& listName, const QString& elementName)
{
    xml.writeStartElement(listName);

    QMap<QString, int> reorderMap;
    for (int i = 0, n = vec.size(); i < n; ++i) {
        reorderMap.insert(vec.at(i).name, i);
    }
    for (auto iter = reorderMap.begin(), iterEnd = reorderMap.end(); iter != iterEnd; ++iter) {
        xml.writeStartElement(elementName);
        vec.at(iter.value()).saveToXML(xml);
    }

    xml.writeEndElement();
}

QString getBoundaryTypeString(SimpleParser::BoundaryType ty)
{
    switch (ty) {
    case SimpleParser::BoundaryType::StringLiteral:                         return XML_STRINGLITERAL;
    case SimpleParser::BoundaryType::Regex:                                 return XML_REGEX;
    case SimpleParser::BoundaryType::SpecialCharacter_OptionalWhiteSpace:   return XML_SPECIAL_CHARACTER_WHITESPACE_OPTIONAL;
    case SimpleParser::BoundaryType::SpecialCharacter_WhiteSpaces:          return XML_SPECIAL_CHARACTER_WHITESPACES;
    case SimpleParser::BoundaryType::SpecialCharacter_LineFeed:             return XML_SPECIAL_CHARACTER_LINEFEED;
    case SimpleParser::BoundaryType::Concatenation:                         return XML_CONCATENATION;
    case SimpleParser::BoundaryType::ClassBased:                            return XML_CLASSBASED;
    }
    return QString();
}
std::initializer_list<QString> getBoundaryTypeStrings()
{
    return std::initializer_list<QString>({
        XML_STRINGLITERAL,
        XML_REGEX,
        XML_SPECIAL_CHARACTER_WHITESPACE_OPTIONAL,
        XML_SPECIAL_CHARACTER_WHITESPACES,
        XML_SPECIAL_CHARACTER_LINEFEED,
        XML_CONCATENATION,
        XML_CLASSBASED
    });
}

bool getBoundaryTypeFromString(QStringRef str, SimpleParser::BoundaryType& ty)
{
    if (str == XML_STRINGLITERAL) {
        ty = SimpleParser::BoundaryType::StringLiteral;
    } else if (str == XML_REGEX) {
        ty = SimpleParser::BoundaryType::Regex;
    } else if (str == XML_SPECIAL_CHARACTER_WHITESPACE_OPTIONAL) {
        ty = SimpleParser::BoundaryType::SpecialCharacter_OptionalWhiteSpace;
    } else if (str == XML_SPECIAL_CHARACTER_WHITESPACES) {
        ty = SimpleParser::BoundaryType::SpecialCharacter_WhiteSpaces;
    } else if (str == XML_SPECIAL_CHARACTER_LINEFEED) {
        ty = SimpleParser::BoundaryType::SpecialCharacter_LineFeed;
    } else if (str == XML_CONCATENATION) {
        ty = SimpleParser::BoundaryType::Concatenation;
    } else if (str == XML_CLASSBASED) {
        ty = SimpleParser::BoundaryType::ClassBased;
    } else {
        return false;
    }
    return true;
}
}

void SimpleParser::Data::saveToXML_NoTerminate(QXmlStreamWriter& xml) const
{
    writeSortedVec(xml, matchRuleNodes,     XML_MATCH_RULE_NODE_LIST,       XML_MATCH_RULE_NODE);
    writeSortedVec(xml, namedBoundaries,    XML_NAMED_BOUNDARY_LIST,        XML_NAMED_BOUNDARY);
    writeSortedVec(xml, contentTypes,       XML_CONTENT_TYPE_LIST,          XML_CONTENT_TYPE);
    writeSortedVec(xml, parenthesis,        XML_BALANCED_PARENTHESIS_LIST,  XML_PARENTHESIS);
    XMLUtil::writeStringList(xml, topNodeList, XML_TOP_NODE_LIST, XML_NODE, true);
    XMLUtil::writeStringList(xml, whitespaceList, XML_WHITESPACE_LIST, XML_WHITESPACE, true);
    XMLUtil::writeFlagElement(xml, {{flag_skipEmptyLineBeforeMatching, XML_FLAG_SKIP_EMPTY_LINE_BEFORE_MATCHING}}, XML_FLAGS);
}

void SimpleParser::Data::saveToXML(QXmlStreamWriter& xml) const
{
    saveToXML_NoTerminate(xml);
    xml.writeEndElement();
}

bool SimpleParser::Data::loadFromXML_NoTerminate(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleParser::Data";
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_MATCH_RULE_NODE_LIST, XML_MATCH_RULE_NODE, matchRuleNodes, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_NAMED_BOUNDARY_LIST, XML_NAMED_BOUNDARY, namedBoundaries, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_CONTENT_TYPE_LIST, XML_CONTENT_TYPE, contentTypes, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_BALANCED_PARENTHESIS_LIST, XML_PARENTHESIS, parenthesis, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_TOP_NODE_LIST, XML_NODE, topNodeList, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_WHITESPACE_LIST, XML_WHITESPACE, whitespaceList, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readFlagElement(xml, curElement, {{flag_skipEmptyLineBeforeMatching, XML_FLAG_SKIP_EMPTY_LINE_BEFORE_MATCHING}}, XML_FLAGS, strCache))) {
        return false;
    }
    return true;
}

bool SimpleParser::Data::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    if (Q_UNLIKELY(!loadFromXML_NoTerminate(xml, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleParser::MatchRuleNode::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeAttribute(XML_NAME, name);
    XMLUtil::writeStringList(xml, childNodeNameList, XML_CHILD_LIST, XML_CHILD, true);
    XMLUtil::writeLoadableList(xml, patterns, XML_PATTERN_LIST, XML_PATTERN);
    xml.writeEndElement();
}

bool SimpleParser::MatchRuleNode::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleParser::MatchRuleNode";
    if (Q_UNLIKELY(!XMLUtil::readStringAttribute(xml, curElement, QString(), XML_NAME, name, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_CHILD_LIST, XML_CHILD, childNodeNameList, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_PATTERN_LIST, XML_PATTERN, patterns, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleParser::Pattern::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeAttribute(XML_TYPENAME, typeName);
    if (patternPass != 0) {
        xml.writeAttribute(XML_PASS, QString::number(patternPass));
    }

    XMLUtil::writeLoadableList(xml, pattern, XML_PATTERN_ELEMENT_LIST, XML_PATTERN_ELEMENT);
    xml.writeEndElement();
}

bool SimpleParser::Pattern::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleParser::Pattern";
    if (Q_UNLIKELY(!XMLUtil::readStringAttribute(xml, curElement, QString(), XML_TYPENAME, typeName, strCache))) {
        return false;
    }
    patternPass = 0;
    if (Q_UNLIKELY(!XMLUtil::readOptionalIntAttribute(xml, curElement, QString(), XML_PASS, patternPass))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_PATTERN_ELEMENT_LIST, XML_PATTERN_ELEMENT, pattern, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleParser::PatternElement::saveToXML(QXmlStreamWriter& xml) const
{
    switch (ty) {
    case ElementType::AnonymousBoundary_StringLiteral: {
        xml.writeAttribute(XML_PATTERN_ELEMENT_TYPE, XML_STRINGLITERAL);
    }break;
    case ElementType::AnonymousBoundary_Regex: {
        xml.writeAttribute(XML_PATTERN_ELEMENT_TYPE, XML_REGEX);
    }break;
    case ElementType::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace: {
        xml.writeAttribute(XML_PATTERN_ELEMENT_TYPE, XML_SPECIAL_CHARACTER_WHITESPACE_OPTIONAL);
    }break;
    case ElementType::AnonymousBoundary_SpecialCharacter_WhiteSpaces: {
        xml.writeAttribute(XML_PATTERN_ELEMENT_TYPE, XML_SPECIAL_CHARACTER_WHITESPACES);
    }break;
    case ElementType::AnonymousBoundary_SpecialCharacter_LineFeed: {
        xml.writeAttribute(XML_PATTERN_ELEMENT_TYPE, XML_SPECIAL_CHARACTER_LINEFEED);
    }break;
    case ElementType::NamedBoundary: {
        xml.writeAttribute(XML_PATTERN_ELEMENT_TYPE, XML_NAMED_BOUNDARY);
    }break;
    case ElementType::Content: {
        xml.writeAttribute(XML_PATTERN_ELEMENT_TYPE, XML_CONTENT);
    }break;
    }
    xml.writeTextElement(XML_STRING, str);
    xml.writeTextElement(XML_PATTERN_ELEMENT_EXPORT_NAME, elementName);
    xml.writeEndElement();
}

bool SimpleParser::PatternElement::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    auto elementTypeReadCB = [this](QStringRef tyStr) -> bool {
        if (tyStr == XML_STRINGLITERAL) {
            ty = ElementType::AnonymousBoundary_StringLiteral;
        } else if (tyStr == XML_REGEX) {
            ty = ElementType::AnonymousBoundary_Regex;
        } else if (tyStr == XML_SPECIAL_CHARACTER_WHITESPACE_OPTIONAL) {
            ty = ElementType::AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace;
        } else if (tyStr == XML_SPECIAL_CHARACTER_WHITESPACES) {
            ty = ElementType::AnonymousBoundary_SpecialCharacter_WhiteSpaces;
        } else if (tyStr == XML_SPECIAL_CHARACTER_LINEFEED) {
            ty = ElementType::AnonymousBoundary_SpecialCharacter_LineFeed;
        } else if (tyStr == XML_NAMED_BOUNDARY) {
            ty = ElementType::NamedBoundary;
        } else if (tyStr == XML_CONTENT) {
            ty = ElementType::Content;
        } else {
            return false;
        }
        return true;
    };
    const char* curElement = "SimpleParser::PatternElement";
    if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_PATTERN_ELEMENT_TYPE, elementTypeReadCB, {XML_STRINGLITERAL, XML_REGEX, XML_NAMED_BOUNDARY, XML_CONTENT}, nullptr))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_STRING, str, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_PATTERN_ELEMENT_EXPORT_NAME, elementName, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleParser::NamedBoundary::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeAttribute(XML_TYPE, getBoundaryTypeString(ty));
    xml.writeTextElement(XML_NAME, name);
    XMLUtil::writeLoadableList(xml, elements, XML_ELEMENT_LIST, XML_ELEMENT);
    if (ty == BoundaryType::ClassBased) {
        XMLUtil::writeStringList(xml, parentList, XML_PARENT_LIST, XML_PARENT, true);
    }
    xml.writeEndElement();
}

bool SimpleParser::NamedBoundary::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    auto getTypeCB = [this](QStringRef text) -> bool {
        return getBoundaryTypeFromString(text, ty);
    };
    const char* curElement = "SimpleParser::NamedBoundary";
    if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_TYPE, getTypeCB, getBoundaryTypeStrings(), nullptr))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_NAME, name, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readLoadableList(xml, curElement, XML_ELEMENT_LIST, XML_ELEMENT, elements, strCache))) {
        return false;
    }
    if (ty == BoundaryType::ClassBased) {
        if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_PARENT_LIST, XML_PARENT, parentList, strCache))) {
            return false;
        }
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleParser::BoundaryDeclaration::saveToXML(QXmlStreamWriter& xml) const
{
    switch (decl) {
    case DeclarationType::Value: {
        xml.writeAttribute(XML_DECLARATION_TYPE, XML_VALUE);
        xml.writeAttribute(XML_TYPE, getBoundaryTypeString(ty));
        xml.writeTextElement(XML_STRING, str);
    }break;
    case DeclarationType::NameReference: {
        xml.writeAttribute(XML_DECLARATION_TYPE, XML_NAME_REFERENCE);
        xml.writeTextElement(XML_NAME, str);
    }break;
    }
    xml.writeEndElement();
}

bool SimpleParser::BoundaryDeclaration::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    auto getDeclTypeCB = [this](QStringRef text) -> bool {
        if (text == XML_VALUE) {
            decl = DeclarationType::Value;
        } else if (text == XML_NAME_REFERENCE) {
            decl = DeclarationType::NameReference;
        } else {
            return false;
        }
        return true;
    };
    const char* curElement = "SimpleParser::BoundaryDeclaration";
    if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_DECLARATION_TYPE, getDeclTypeCB, {XML_VALUE, XML_NAME_REFERENCE}, nullptr))) {
        return false;
    }
    switch (decl) {
    case DeclarationType::Value: {
        auto getBoundaryTypeCB = [this](QStringRef text) -> bool {
            return getBoundaryTypeFromString(text, ty);
        };
        if (Q_UNLIKELY(!XMLUtil::readAttribute(xml, curElement, QString(), XML_TYPE, getBoundaryTypeCB, getBoundaryTypeStrings(), nullptr))) {
            return false;
        }
        if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_STRING, str, strCache))) {
            return false;
        }
    }break;
    case DeclarationType::NameReference: {
        if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_NAME, str, strCache))) {
            return false;
        }
    }break;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleParser::ContentType::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeTextElement(XML_NAME, name);
    XMLUtil::writeStringList(xml, acceptedParenthesisNest, XML_CONTENT_ACCEPTED_PARENTHESIS_LIST, XML_PARENTHESIS, true);
    xml.writeEndElement();
}

bool SimpleParser::ContentType::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleParser::ContentType";
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_NAME, name, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_CONTENT_ACCEPTED_PARENTHESIS_LIST, XML_PARENTHESIS, acceptedParenthesisNest, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

void SimpleParser::BalancedParenthesis::saveToXML(QXmlStreamWriter& xml) const
{
    xml.writeTextElement(XML_NAME, name);
    xml.writeTextElement(XML_OPEN, open);
    xml.writeTextElement(XML_CLOSE, close);
    xml.writeEndElement();
}

bool SimpleParser::BalancedParenthesis::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleParser::BalancedParenthesis";
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_NAME, name, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_OPEN, open, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_CLOSE, close, strCache))) {
        return false;
    }
    xml.skipCurrentElement();
    return true;
}

// -----------------------------------------------------------------------------

const SimpleParserEvent SimpleParserEvent::interp;

const EventInterpreter* SimpleParserEvent::getInterpreter()
{
    return &interp;
}

QString SimpleParserEvent::getString(const EventLogger* logger, const Event& e, InterpretedStringType ty)
{
    // the events must have been populated by SimpleParserEvent::* with * having the same name as enum value below
    // Note that if parsing is successful, node creation events will also contain mapping from source text to dest tree node
    int eventTypeIndex = e.eventTypeIndex;
    switch (eventTypeIndex) {
    default: {
        QMetaEnum eventIDEnum = QMetaEnum::fromType<EventID>();
        return QString(eventIDEnum.valueToKey(eventTypeIndex));
    }break;
    case static_cast<int>(EventID::EmptyLineSkipped): {
        int startLineNum = e.data.at(0).toInt();
        int endLineNum = e.data.at(1).toInt();
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            if (startLineNum == endLineNum) {
                return tr("Empty line skipped (line %1)").arg(startLineNum);
            } else {
                return tr("Empty line skipped (line %1~%2)").arg(QString::number(startLineNum), QString::number(endLineNum));
            }
        }break;
        case InterpretedStringType::Detail: {
            return tr("The parser will skip empty lines after a pattern is matched. If your use case require matching with these empty lines, please integrate the empty line into the previous pattern getting matched.");
        }break;
        }
    }break;
        /*
    case static_cast<int>(EventID::FramePushed): {
        QString parentRuleName = e.data.front().toString();
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            return tr("Frame Pushed (%1)").arg(parentRuleName);
        }break;
        case InterpretedStringType::Detail: {
            QString baseTest = tr("A pattern under the rule \"%1\" (current rule) is matched and created a node (current node). "
                                  "Because the current rule has child rule(s), the parser created a \"frame\" for this node, which:\n"
                                  " 1. enable all the patterns from all the child rule(s) for subsequent matching\n"
                                  " 2. new nodes created from matching the enabled patterns will be placed under the current node in the output tree\n"
                                  "If a rule is not listed as expected, please add the source rule to the parent list of the missing rule.").arg(parentRuleName);
            QString childRuleString = tr("%1 rule nodes are enabled").arg(e.data.size()-1);
            if (e.data.size() < 4) {
                childRuleString.append(" (");
                for (int i = 1; i < e.data.size(); ++i) {
                    QString ruleName = e.data.at(i).toString();
                    childRuleString.append(ruleName);
                    if (i + 1 < e.data.size()) {
                        childRuleString.append(", ");
                    }
                }
                childRuleString.append(")");
            };
            return baseTest + "\n" + childRuleString;
        }break;
        }
    }break;
    case static_cast<int>(EventID::FramePoped): {
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            return tr("Frame Poped");
        }break;
        case InterpretedStringType::Detail: {
            return tr("None of the patterns enabled in the frame was matched. Therefore the frame was removed and patterns enabled in the previous frame would be attempted.\n"
                      "If the expected pattern was not attempted, please check whether the containing rule has added the frame's current rule as parent.\n"
                      "If a pattern was attempted but failed to match, please check the details on the corresponding matching failure event.");
        }break;
        }
    }break;
        */
    case static_cast<int>(EventID::FramePopedForEarlyExit): {
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            return tr("Frame Poped (Explicit)");
        }break;
        case InterpretedStringType::Detail: {
            return tr("When a pattern from the frame's rule with empty output node type name is matched, the frame will be popped immediately.\n"
                      "If the frame popping action is unexpected, please correct the output node type name for the matching pattern.\n"
                      "If the pattern matching is unexpected, please check the referenced matching events to identify the cause of unexpected match.");
        }break;
        }
    }break;
    case static_cast<int>(EventID::RootNodeNoFrame): {
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            return tr("Root Node Has No Frame");
        }break;
        case InterpretedStringType::Detail: {
            return tr("The root rule does not have any child rule. Therefore no frame is created for the root node.\n"
                      "This is only correct if the parser is intended to only create a single node.\n"
                      "If this is unexpected, please check if all intended child rules have added the root rule to the parent list.");
        }break;
        }
    }break;
    case static_cast<int>(EventID::NodeAdded): {
        QString nodeTitle = e.data.front().toString();
        Q_ASSERT(e.locationRemarks.size() <= 2);
        if (e.locationRemarks.size() == 2) {
            // the node position is successfully registered
            const auto& outputStart = e.locationRemarks.at(1);
            Q_ASSERT(outputStart.locationContextIndex == static_cast<int>(EventLocationContext::OutputData));
            int nodeIndex = outputStart.location.value<Tree::LocationType>().nodeIndex;
            nodeTitle.prepend(QString("[%1] ").arg(nodeIndex));
        }
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            return tr("Node Added: ") + nodeTitle;
        }break;
        case InterpretedStringType::Detail: {
            return tr("The node is created from a pattern matching. If this is unexpected, please check the referenced events.");
        }break;
        }
    }break;
    case static_cast<int>(EventID::MatchFinished): {
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            return tr("Pattern Match Finished");
        }break;
        case InterpretedStringType::Detail: {
            return tr("All the pattern matching is finished because either all the input text has been processed or the last frame is popped.");
        }break;
        }
    }break;
    case static_cast<int>(EventID::GarbageAtEnd): {
        switch (ty) {
        case InterpretedStringType::EventTitle: {
            return tr("Unprocessed Text Left");
        }break;
        case InterpretedStringType::Detail: {
            return tr("Non-whitespace text is found after the pattern matching.\n"
                      "If the text is expected to be pattern-matched, "
                        "please check the related frame push and pop events to find whether the pattern is attempted, "
                        "and then either look for the cause of missed attempt or the cause of failed pattern match");
        }break;
        }
    }break;
    }
    return QStringLiteral("<Unimplemented>");
}

QString SimpleParserEvent::getEventTitle           (const EventLogger* logger, int eventIndex, int eventTypeIndex) const
{

    const Event& e = logger->getEvent(eventIndex);
    Q_ASSERT(e.eventTypeIndex == eventTypeIndex);
    return getString(logger, e, InterpretedStringType::EventTitle);
}

QString SimpleParserEvent::getDetailString         (const EventLogger* logger, int eventIndex) const
{
    const Event& e = logger->getEvent(eventIndex);
    return getString(logger, e, InterpretedStringType::Detail);
}

QString SimpleParserEvent::getReferenceTypeTitle   (const EventLogger* logger, int eventIndex, int eventTypeIndex, int referenceTypeIndex) const
{
    Q_UNUSED(logger)
    Q_UNUSED(eventIndex)
    Q_UNUSED(eventTypeIndex)
    QMetaEnum eventReferenceIDMeta = QMetaEnum::fromType<EventReferenceID>();
    return QString(eventReferenceIDMeta.valueToKey(referenceTypeIndex));
}

QString SimpleParserEvent::getLocationTypeTitle    (const EventLogger* logger, int eventIndex, int eventTypeIndex, int locationIndex) const
{
    Q_UNUSED(logger)
    Q_UNUSED(eventIndex)
    switch (eventTypeIndex) {
    case static_cast<int>(EventID::NodeAdded): {
        switch (locationIndex) {
        default: qFatal("Unexpected location index for NodeAdded event"); return QString();
        case 0: {
            return tr("Source Text");
        }break;
        case 1: {
            return tr("Result Node");
        }break;
        }
    }break;
    default: {
        Q_ASSERT(locationIndex == 0);
        return QString();
    }
    }
}
