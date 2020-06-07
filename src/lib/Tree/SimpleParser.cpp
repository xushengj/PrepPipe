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
    QHash<QString, int> matchRuleNodeNameToIndexMap;
    for (int i = 0, n = d.matchRuleNodes.size(); i < n; ++i) {
        const auto& rule = d.matchRuleNodes.at(i);
        matchRuleNodeNameToIndexMap.insert(rule.name, i);
    }
    childNodeMatchRuleVec.resize(d.matchRuleNodes.size());
    for (int i = 0, n = d.matchRuleNodes.size(); i < n; ++i) {
        const auto& rule = d.matchRuleNodes.at(i);
        for (const auto& parent : rule.parentNodeNameList) {
            int parentIndex = matchRuleNodeNameToIndexMap.value(parent, -1);
            Q_ASSERT(parentIndex != -1);
            childNodeMatchRuleVec[parentIndex].push_back(i);
        }
    }
    rootNodeRuleIndex = matchRuleNodeNameToIndexMap.value(d.rootRuleNodeName, -1);
    Q_ASSERT(rootNodeRuleIndex != -1);
}

bool SimpleParser::performParsing(const QString& src, Tree& dest)
{
    // SimpleParser should be in clean state when entering
    state.set(src);

    struct RuleStackFrame {
        TreeBuilder::Node* ptr = nullptr;
        QVector<int> childMatchRules;

        RuleStackFrame() = default;
        RuleStackFrame(TreeBuilder::Node* node, const QVector<int>& child)
            : ptr(node), childMatchRules(child)
        {}
        RuleStackFrame(const RuleStackFrame&) = default;
        RuleStackFrame(RuleStackFrame&&) = default;
    };
    QVector<RuleStackFrame> ruleStack;

    int pos = 0;
    int length = src.length();

    auto skipEmptyLines = [&]() -> void {
        QStringRef str = src.midRef(pos);
        auto match = emptyLineRegex.match(str, 0, QRegularExpression::NormalMatch, QRegularExpression::AnchoredMatchOption);
        if (match.hasMatch()) {
            int matchStart = match.capturedStart();
            int matchEnd = match.capturedEnd();
            Q_ASSERT(matchStart == 0);
            int dist = matchEnd;
            pos += dist;
            state.curPosition = pos;
        }
    };

    // bootstrap the first node
    {
        const MatchRuleNode& rootRule = data.matchRuleNodes.at(rootNodeRuleIndex);
        TreeBuilder::Node* root = nullptr;

        if (rootRule.patterns.isEmpty()) {
            // just create a node with root rule's name as type
            root = builder.addNode(nullptr);
            root->typeName = rootRule.name;
        } else {
            // try all patterns
            if (data.flag_skipEmptyLineBeforeMatching) {
                skipEmptyLines();
            }
            PatternMatchResult curBestResult;
            for (const auto& p : rootRule.patterns) {
                PatternMatchResult curResult = tryPattern(p, pos);
                if (!curResult) {
                    continue;
                }
                if (!curBestResult || curResult.isBetterThan(curBestResult)) {
                    curBestResult = curResult;
                }
            }
            if (!curBestResult) {
                return false;
            }
            pos += curBestResult.totalConsumedLength;
            root = curBestResult.node;
        }
        const auto& rootChildRules = childNodeMatchRuleVec.at(rootNodeRuleIndex);
        if (!rootChildRules.isEmpty()) {
            ruleStack.push_back(RuleStackFrame(root, rootChildRules));
        }
    }

    // main loop
    while (!ruleStack.isEmpty() && (pos < length)) {
        const auto& frame = ruleStack.back();

        PatternMatchResult curBestResult;
        int childNodeRuleIndex = -1;

        if (data.flag_skipEmptyLineBeforeMatching) {
            skipEmptyLines();
        }

        for (int childRuleIndex : frame.childMatchRules) {
            const MatchRuleNode& rule = data.matchRuleNodes.at(childRuleIndex);
            for (const auto& p : rule.patterns) {
                PatternMatchResult curResult = tryPattern(p, pos);
                if (!curResult) {
                    continue;
                }
                if (!curBestResult || curResult.isBetterThan(curBestResult)) {
                    curBestResult = curResult;
                    childNodeRuleIndex = childRuleIndex;
                }
            }
        }

        if (childNodeRuleIndex == -1) {
            // no matches in current node rule
            // go to parent
            ruleStack.pop_back();
            continue;
        }

        TreeBuilder::Node* node = curBestResult.node;
        Q_ASSERT(node);

        pos += curBestResult.totalConsumedLength;
        state.curPosition = pos;

        // if the best match is an early exit pattern, we also go to parent
        if (curBestResult.node->typeName.isEmpty()) {
            ruleStack.pop_back();
            continue;
        }

        // we found the best match
        TreeBuilder::Node* parent = frame.ptr;
        node->setParent(parent);

        const auto& childRules = childNodeMatchRuleVec.at(childNodeRuleIndex);
        if (!childRules.isEmpty()) {
            ruleStack.push_back(RuleStackFrame(node, childRules));
        }
    }

    // okay, done; no further match possible
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
        // TODO generate a warning
        return false;
    }

    Tree tree(builder);
    dest.swap(tree);

    state.clear();
    builder.clear();

    return true;
}

void SimpleParser::ParseState::clear()
{
    stringLiteralPositionMap.clear();
    regexMatchPositionMap.clear();
    str = nullptr;

    // the following two is persistent across uses
    // regexPatternToIndexMap.clear();
    // regexList.clear();
}

void SimpleParser::ParseState::set(const QString& text)
{
    str = &text;
    strLength = text.length();
    curPosition = 0;
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

SimpleParser::PatternMatchResult SimpleParser::tryPattern(const Pattern& pattern, int position)
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
        int contentIndex = -1;
        QString contentExportName;
        QString boundaryExportName;
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
            nextElementIndex += 1;
            Q_ASSERT(nextPE.ty != PatternElement::ElementType::Content);
            boundaryExportName = nextPE.elementName;
            setDeclByPatternElement(decl, nextPE);
        } else {
            // this is a boundary
            boundaryExportName = pe.elementName;
            setDeclByPatternElement(decl, pe);
        }

        int currentPosition = position + totalConsumeCount;
        std::pair<int, int> pos = findBoundary(currentPosition, decl, contentIndex);
        if (pos.first < 0) {
            // no matches
            return result;
        }

        // okay, we have a match
        totalConsumeCount += pos.first + pos.second;
        boundaryConsumeCount += pos.second;

        if (contentIndex == -1) {
            Q_ASSERT(pos.first == 0);
        } else {
            if (!contentExportName.isEmpty()) {
                keyList.push_back(contentExportName);
                valueList.push_back(std::make_pair(currentPosition, pos.first));
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
    return result;
}

std::pair<int, int> SimpleParser::findBoundary(int pos, const BoundaryDeclaration& decl, int precedingContentTypeIndex)
{
    switch (decl.decl) {
    case BoundaryDeclaration::DeclarationType::Value: {
        switch (decl.ty) {
        case BoundaryType::StringLiteral:   return findBoundary_StringLiteral   (pos, decl.str, precedingContentTypeIndex);
        case BoundaryType::Regex:           return findBoundary_Regex           (pos, decl.str, precedingContentTypeIndex);

        case BoundaryType::SpecialCharacter_OptionalWhiteSpace: return findBoundary_SpecialCharacter_OptionalWhiteSpace (pos, precedingContentTypeIndex);
        case BoundaryType::SpecialCharacter_WhiteSpaces:        return findBoundary_SpecialCharacter_WhiteSpaces        (pos, precedingContentTypeIndex);
        case BoundaryType::SpecialCharacter_LineFeed:           return findBoundary_SpecialCharacter_LineFeed           (pos, precedingContentTypeIndex);

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
            std::pair<int, int> firstResult = findBoundary(pos, b.elements.front(), precedingContentTypeIndex);
            if (firstResult.first < 0) {
                return std::make_pair(-1, 0);
            }
            int consumeCount = firstResult.first + firstResult.second;
            for (int i = 1, n = b.elements.size(); i < n; ++i) {
                std::pair<int, int> result = findBoundary(pos + consumeCount, b.elements.at(i), -1);
                Q_ASSERT(result.first <= 0);
                if (result.first == 0) {
                    consumeCount += result.second;
                } else {
                    return std::make_pair(-1, 0);
                }
            }
            return std::make_pair(firstResult.first, consumeCount - firstResult.first);
        }break;
        case BoundaryType::ClassBased: return findBoundary_ClassBased(pos, index, precedingContentTypeIndex);
        default: {
            qFatal("Unhandled named boundary");
        }break;
        }
    }
    }
    Q_UNREACHABLE();
}

std::pair<int, int> SimpleParser::findBoundary_ClassBased(int pos, int boundaryIndex, int precedingContentTypeIndex)
{
    const NamedBoundary& boundary = data.namedBoundaries.at(boundaryIndex);
    Q_ASSERT(boundary.ty == BoundaryType::ClassBased);
    std::pair<int, int> bestResult(-1, 0);
    for (const auto& decl : boundary.elements) {
        std::pair<int, int> curResult = findBoundary(pos, decl, precedingContentTypeIndex);
        if (curResult.first < 0)
            continue;
        if ((bestResult.first == -1) || (bestResult.first > curResult.first) || (bestResult.first == curResult.first && bestResult.second < curResult.second)) {
            bestResult = curResult;
        }
    }
    auto iter = classBasedBoundaryChildList.find(boundaryIndex);
    Q_ASSERT(iter != classBasedBoundaryChildList.end());
    for (int child : iter.value()) {
        std::pair<int, int> curResult = findBoundary_ClassBased(pos, child, precedingContentTypeIndex);
        if (curResult.first < 0)
            continue;
        if ((bestResult.first == -1) || (bestResult.first > curResult.first) || (bestResult.first == curResult.first && bestResult.second < curResult.second)) {
            bestResult = curResult;
        }
    }

    return bestResult;
}

int SimpleParser::contentCheck(const ContentType& content, int startPos, int length)
{
    // TODO
    Q_UNUSED(content)
    Q_UNUSED(startPos)
    Q_UNUSED(length)
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

        return std::make_pair(startOffset, length);
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

std::pair<int, int> SimpleParser::findBoundary_StringLiteral(int pos, const QString& str, int precedingContentTypeIndex)
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
        int result = contentCheck(c, curPos, dist);
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

std::pair<int, int> SimpleParser::findBoundary_Regex(int pos, const QString& str, int precedingContentTypeIndex)
{
    return findBoundary_Regex_Impl(pos, state.getRegexIndex(str), precedingContentTypeIndex);
}

std::pair<int, int> SimpleParser::findBoundary_SpecialCharacter_OptionalWhiteSpace(int pos, int precedingContentTypeIndex)
{
    // it seems that if the next character is \n, it will give "no match" result
    // add workaround here
    std::pair<int, int> result = findBoundary_Regex_Impl(pos, regexIndex_SpecialCharacter_OptionalWhiteSpace, precedingContentTypeIndex);
    if (result.first == -1) {
        return std::make_pair(0, 0);
    }
    return result;
}

std::pair<int, int> SimpleParser::findBoundary_SpecialCharacter_WhiteSpaces(int pos, int precedingContentTypeIndex)
{
    return findBoundary_Regex_Impl(pos, regexIndex_SpecialCharacter_WhiteSpaces, precedingContentTypeIndex);
}

std::pair<int, int> SimpleParser::findBoundary_SpecialCharacter_LineFeed(int pos, int precedingContentTypeIndex)
{
    return findBoundary_StringLiteral(pos, QStringLiteral("\n"), precedingContentTypeIndex);
}

std::pair<int, int> SimpleParser::findBoundary_Regex_Impl(int pos, int regexIndex, int precedingContentTypeIndex)
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
    int firstResult = contentCheck(c, pos, dists.first);
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
        int result = contentCheck(c, curPos, curDist.second);
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
const QString XML_ROOT_NODE = QStringLiteral("RootMatchRuleNode");

const QString XML_TYPE = QStringLiteral("Type");
const QString XML_NAME = QStringLiteral("Name");
const QString XML_PARENT_LIST = QStringLiteral("Parents");
const QString XML_PARENT = QStringLiteral("Parent");
const QString XML_PATTERN_LIST = QStringLiteral("Patterns");
const QString XML_PATTERN = QStringLiteral("Pattern");

const QString XML_TYPENAME = QStringLiteral("TypeName");
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
    xml.writeTextElement(XML_ROOT_NODE, rootRuleNodeName);
    writeSortedVec(xml, matchRuleNodes,     XML_MATCH_RULE_NODE_LIST,       XML_MATCH_RULE_NODE);
    writeSortedVec(xml, namedBoundaries,    XML_NAMED_BOUNDARY_LIST,        XML_NAMED_BOUNDARY);
    writeSortedVec(xml, contentTypes,       XML_CONTENT_TYPE_LIST,          XML_CONTENT_TYPE);
    writeSortedVec(xml, parenthesis,        XML_BALANCED_PARENTHESIS_LIST,  XML_PARENTHESIS);
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
    if (Q_UNLIKELY(!XMLUtil::readString(xml, curElement, XML_ROOT_NODE, rootRuleNodeName, strCache))) {
        return false;
    }
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
    XMLUtil::writeStringList(xml, parentNodeNameList, XML_PARENT_LIST, XML_PARENT, true);
    XMLUtil::writeLoadableList(xml, patterns, XML_PATTERN_LIST, XML_PATTERN);
    xml.writeEndElement();
}

bool SimpleParser::MatchRuleNode::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleParser::MatchRuleNode";
    if (Q_UNLIKELY(!XMLUtil::readStringAttribute(xml, curElement, QString(), XML_NAME, name, strCache))) {
        return false;
    }
    if (Q_UNLIKELY(!XMLUtil::readStringList(xml, curElement, XML_PARENT_LIST, XML_PARENT, parentNodeNameList, strCache))) {
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
    XMLUtil::writeLoadableList(xml, pattern, XML_PATTERN_ELEMENT_LIST, XML_PATTERN_ELEMENT);
    xml.writeEndElement();
}

bool SimpleParser::Pattern::loadFromXML(QXmlStreamReader& xml, StringCache& strCache)
{
    const char* curElement = "SimpleParser::Pattern";
    if (Q_UNLIKELY(!XMLUtil::readStringAttribute(xml, curElement, QString(), XML_TYPENAME, typeName, strCache))) {
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
