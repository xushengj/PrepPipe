#ifndef SIMPLEPARSER_H
#define SIMPLEPARSER_H

#include "src/GlobalInclude.h"
#include "src/lib/Tree/Tree.h"
#include "src/lib/Tree/EventLogging.h"
#include "src/utils/XMLUtilities.h"
#include "src/utils/TextUtilities.h"

#include <QString>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QRegularExpression>
#include <QCoreApplication>

class SimpleParser
{
public:
    // boundary: the expression that marks boundary of free content and we can search for in a string
    // two basic types:
    //      Anonymous boundary: StringLiteral or Regex that are unique'd by content
    //      Named boundary: other more complex expressions that are refered by name
    enum class BoundaryType : int{
        // ----------------------------------------------------------------
        // Anonymous boundary types

        StringLiteral,
        Regex,

        // special characters use distinct enum value to avoid the need of testing StringLiteral against all these characters for "user friendly" gui
        SpecialCharacter_OptionalWhiteSpace, // 0 or more white space, or tab; probably display as normal whitespace
        SpecialCharacter_WhiteSpaces, // (>=1) white space, or tab; probably display as open box (U+2423)
        SpecialCharacter_LineFeed, // probably display as return symbol (U+23CE)
        // ----------------------------------------------------------------
        // Named boundaries

        Concatenation, // concatenation of anonymous boundaries

        ClassBased// based on inheritance

    };
    struct BoundaryDeclaration {
        enum class DeclarationType : int {
            Value, // this declaration is an anonymous child that have entire data stored in this declaration
            NameReference // this declaration only makes a name reference to child
        };
        DeclarationType decl = DeclarationType::Value;
        BoundaryType ty = BoundaryType::StringLiteral;
        QString str;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct NamedBoundary {
        BoundaryType ty = BoundaryType::Concatenation;
        QString name;

        QVector<BoundaryDeclaration> elements;
        QStringList parentList; // used by inheritance based boundary type to populate all its parents

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    // content: the part of text that we can validate but not actively find; the range is determined by surrounding boundaries
    struct ContentType {
        QString name;
        // later on we may add validation policies here
        // for now we are only interested in what paranthesis nesting can happen here
        // TODO acceptedParenthesisNest is not implemented in parsing yet
        QStringList acceptedParenthesisNest; // list of names of BalancedParenthesis

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct BalancedParenthesis {
        QString name;
        QString open;
        QString close;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct PatternElement {
        enum class ElementType {
            // note that all anonymous boundary should be listed here
            AnonymousBoundary_StringLiteral,
            AnonymousBoundary_Regex,
            AnonymousBoundary_SpecialCharacter_OptionalWhiteSpace,
            AnonymousBoundary_SpecialCharacter_WhiteSpaces,
            AnonymousBoundary_SpecialCharacter_LineFeed,
            NamedBoundary,
            Content
        };
        ElementType ty = ElementType::AnonymousBoundary_StringLiteral;
        QString str; // str for AnonymousBoundary; name for anything else
        QString elementName; // name of element for export; empty to ignore it without export

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct Pattern {
        QString typeName;   // type name of generated tree node; empty for exit pattern
        QVector<PatternElement> pattern;    // the pattern for matching
        int patternPass = 0;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct MatchRuleNode {
        QString name;
        QStringList childNodeNameList;
        QVector<Pattern> patterns;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct Data {
        QVector<MatchRuleNode> matchRuleNodes;
        QVector<NamedBoundary> namedBoundaries;
        QVector<ContentType> contentTypes; // the first item will be used for anonymous ones
        QVector<BalancedParenthesis> parenthesis;
        QStringList whitespaceList; // even we mean for list of single characters, QChar only has 16 bits and we need a QString to represent arbitrary (say UTF-32) characters
        QStringList topNodeList; // child node names for the pseudo root node
        bool flag_skipEmptyLineBeforeMatching = true;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
        void saveToXML_NoTerminate(QXmlStreamWriter& xml) const;
        bool loadFromXML_NoTerminate(QXmlStreamReader& xml, StringCache& strCache);

        bool validate(QString& err) const;
    };

    struct ParseState {
        const QString* str = nullptr;
        EventLogger* logger = nullptr;

        struct RegexMatchData {
            int length = 0;
            QVector<QStringRef> namedCaptures;
        };
        // cached matching results during parsing
        // key -> start pos -> earliest last pos[, ...]
        QHash<QString, QMap<int, int>> stringLiteralPositionMap;
        QHash<QString, int> regexPatternToIndexMap;
        QList<QRegularExpression> regexList;
        QList<QMap<int, std::pair<int, RegexMatchData>>> regexMatchPositionMap;
        TextUtil::TextPositionInfo posInfo;
        int strLength = 0;
        int curPosition = 0;

        void clear();
        void set(const QString& text, EventLogger* loggerArg);
        int getRegexIndex(const QString& pattern);
    };

    struct PatternMatchFailAttempts {
        indextype failElement;
        int failElementType;
        QString failElementStr;
        QVector<TextUtil::PlainTextLocation> elementMatchPosVec;
    };

public:
    explicit SimpleParser(const Data& d);
    bool performParsing(const QString& src, Tree& dest, EventLogger* logger = nullptr);

    // returns a string in the form of e.g. ( |\t|\u3000)
    static QString getWhiteSpaceRegexPattern(const QStringList& whiteSpaceList);

private:
    // All persistent data should be in Data
    Data data;

private:
    // helper functions

    /**
     * @brief findBoundary find the specified boundary from the given position of text
     * @param text the input string
     * @param decl the boundary to search for
     * @param precedingContentTypeIndex the type index of content that precedes the boundary; -1 if no content is accepted at current position
     * @param chopWSAfterContent whether there is a whitespace pattern after content, making it required to chop off whitespaces
     * @return a pair: <match start index (offset from current head of text), match length>. return (-1, 0) if there is no match
     */
    std::pair<int, int> findBoundary(int pos, const BoundaryDeclaration& decl, int precedingContentTypeIndex, bool chopWSAfterContent);


    std::pair<int, int> findBoundary_StringLiteral(int pos, const QString& str, int precedingContentTypeIndex, bool chopWSAfterContent);
    std::pair<int, int> findBoundary_Regex(int pos, const QString& str, int precedingContentTypeIndex, bool chopWSAfterContent);
    std::pair<int, int> findBoundary_Regex_Impl(int pos, int regexIndex, int precedingContentTypeIndex, bool chopWSAfterContent);
    std::pair<int, int> findBoundary_SpecialCharacter_OptionalWhiteSpace(int pos, int precedingContentTypeIndex, bool chopWSAfterContent);
    std::pair<int, int> findBoundary_SpecialCharacter_WhiteSpaces(int pos, int precedingContentTypeIndex, bool chopWSAfterContent);
    std::pair<int, int> findBoundary_SpecialCharacter_LineFeed(int pos, int precedingContentTypeIndex, bool chopWSAfterContent);
    std::pair<int, int> findBoundary_ClassBased(int pos, int boundaryIndex, int precedingContentTypeIndex, bool chopWSAfterContent);

    int findNextStringMatch(int startPos, const QString& str);
    std::pair<int, int> findNextRegexMatch(int startPos, const QRegularExpression& regex, QMap<int, std::pair<int, ParseState::RegexMatchData> > &positionMap);

    // incremental check (this function would be called on pieces of contents)
    // return -1 if check fails, 0 if passes, positive distance (ret > length) if the content must be extended
    int contentCheck(const ContentType& content, int startPos, int length, bool chopWSAfterContent);

    // for the substring, what's the length of whitespace in the tail
    int getWhitespaceTailChopLength(int startPos, int endPos);

    struct PatternMatchResult {
        TreeBuilder::Node* node = nullptr;
        int totalConsumedLength = 0;
        int boundaryConsumedLength = 0;
        int nodeFinalEvent = -1;

        bool isBetterThan(const PatternMatchResult& rhs) const {
            if (totalConsumedLength > rhs.totalConsumedLength)
                return true;
            if (totalConsumedLength == rhs.totalConsumedLength && boundaryConsumedLength > rhs.boundaryConsumedLength)
                return true;
            return false;
        }

        operator bool() const {
            return node != nullptr;
        }
    };

    // the original entry code for testing a pattern
    PatternMatchResult tryPattern_v1(const Pattern& pattern, int pos, int patternTestSourceEvent);

    // get a list of pattern element indices for the solving order
    std::vector<indextype> getPatternElementSolvingOrder(const Pattern& pattern);

    // return vec of pairs, everything (both arguments and return values are absolute distance from beginning of the text)
    std::vector<std::pair<int, int>> tryMatchPatternElement(const PatternElement& element, int holeLB_abs, int holeUB_abs, bool pinLB, bool pinUB);

    // the up-to-date entry function for testing a pattern
    PatternMatchResult tryPattern(const Pattern& pattern, int pos, int patternTestSourceEvent);

private:
    // "volatile" data that can be recomputed or ones that are just cache

    // derived helper data
    QHash<QString, int> boundaryNameToIndexMap;
    QHash<QString, int> contentTypeNameToIndexMap;
    QHash<int, QVector<int>> classBasedBoundaryChildList;

    // for match rule nodes, there will be a pseudo root node created
    // the pseudo root (match rule) node will have index of d.matchRuleNodes.size()
    struct MatchPassData {
        QHash<int, QVector<int>> patterns; // ruleNodeIndex -> patternIndices
        int pass = 0;
    };
    using ContextMatchRuleData = QVector<MatchPassData>;
    QVector<ContextMatchRuleData> childNodeMatchRules;
    const int rootNodeRuleIndex = 0;
    // for now, white space related special character search is done by regular expressions
    int regexIndex_SpecialCharacter_OptionalWhiteSpace = -1;
    int regexIndex_SpecialCharacter_WhiteSpaces = -1;
    // line feed search is done by string literal

    // empty line skip is done using dedicated regex
    QRegularExpression emptyLineRegex;

    // runtime data
    ParseState state;
    TreeBuilder builder;

};

// making it usable in QVariant
Q_DECLARE_METATYPE(SimpleParser::PatternMatchFailAttempts);

class SimpleParserEvent : public EventInterpreter
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(MyClass)
public:
    enum class EventID: int {
        RootNodeCreation,
        EmptyLineSkipped,
        FramePopedForEarlyExit,
        RootNodeNoFrame,
        NodeAdded,
        MatchPassNoMatching,
        MatchFailed,
        MatchFinished,
        GarbageAtEnd,
        PatternMatched,
        PatternNotMatched
    };
    Q_ENUM(EventID)

    enum class EventReferenceID: int {
        PatternMatch_MatchFinalEvent,
        PatternMatch_SupportiveEvent, // both positive and negative
        PatternBestCandidateComparison_PreviousEvent,
        PatternBestCandidateComparison_NewEvent,
        FramePop_PushEvent,
        FramePop_ExitPatternEvent,
        FramePop_MatchEvent,
        NodeCreation_FrameEvent,
        NodeCreation_MatchEvent,
        NodeCreation_SupportiveEvent,
        MatchPassFail_FrameEvent,
        MatchPassFail_MatchEvent,
        MatchFailed_FrameEvent,
        MatchFailed_PassFailEvent,
        MatchFinished_FrameEvent,
        PositionMapping_NodeEvent,
        PostMatchingCheck_MatchFinishEvent,
        PatternMatched_SourceEvent,
        PatternNotMatched_SourceEvent,
    };
    Q_ENUM(EventReferenceID)

    enum class EventLocationContext: int {
        InputData = 0,
        OutputData = 1
    };
    Q_ENUM(EventLocationContext)


public:
    static const EventInterpreter* getInterpreter();

    virtual QString getEventTitle           (const EventLogger* logger, int eventIndex, int eventTypeIndex) const override;
    virtual QString getDetailString         (const EventLogger* logger, int eventIndex) const override;
    virtual QString getReferenceTypeTitle   (const EventLogger* logger, int eventIndex, int eventTypeIndex, int referenceTypeIndex) const override;
    virtual QString getLocationTypeTitle    (const EventLogger* logger, int eventIndex, int eventTypeIndex, int locationIndex) const override;

    virtual EventImportance getEventImportance (int eventTypeIndex) const override {
        switch (eventTypeIndex) {
        default: break;
        case static_cast<int>(EventID::NodeAdded):
            return EventImportance::AlwaysPrimary;
        }
        return EventImportance::Auto;
    }

private:
    enum class InterpretedStringType {
        EventTitle,
        Detail
    };
    static QString getString(const EventLogger *logger, const Event& e, InterpretedStringType ty);

    static EventLocationRemark getInputLoc(int startPos, int endPos) {
        TextUtil::PlainTextLocation locData;
        locData.startPos = startPos;
        locData.endPos = endPos;
        EventLocationRemark loc;
        loc.location.setValue(locData);
        loc.locationContextIndex = static_cast<int>(EventLocationContext::InputData);
        return loc;
    }
    static QVector<EventLocationRemark> getSingleInputLoc(int startPos, int endPos) {
        QVector<EventLocationRemark> result;
        result.push_back(getInputLoc(startPos, endPos));
        return result;
    }
    static EventLocationRemark getOutputLoc(int nodeIndex) {
        Tree::LocationType locData;
        locData.nodeIndex = nodeIndex;
        EventLocationRemark loc;
        loc.location.setValue(locData);
        loc.locationContextIndex = static_cast<int>(EventLocationContext::OutputData);
        return loc;
    }

private:
    static const SimpleParserEvent interp;
public:
    // transparently skip all logging if we don't have the logger
    template <typename F, typename... ArgTy>
    static int Log(F f, EventLogger* logger, ArgTy&&... args) {
        if (!logger) {
            return -1;
        }
        return f(logger, std::forward<ArgTy>(args)...);
    }

    // functions to actually log events
    static int RootNodeCreation(EventLogger* logger)
    {
        return logger->addEvent(SimpleParserEvent::EventID::RootNodeCreation);
    }

    static int EmptyLineSkipped(EventLogger* logger, int startPos, int endPos, int startLineNum, int endLineNum)
    {
        Q_ASSERT(endLineNum >= startLineNum);
        QVector<EventLocationRemark> locations = getSingleInputLoc(startPos, endPos);
        return logger->addEvent(SimpleParserEvent::EventID::EmptyLineSkipped,
                                QVariantList() << startLineNum << endLineNum, EventColorOption::Passive, QVector<EventReference>(), locations);
    }

    static int FramePopedForEarlyExit(EventLogger* logger, int frameEvent, int patternEvent, const QList<int>& positiveMatchEvent, const QList<int>& negativeMatchEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::FramePop_PushEvent, frameEvent);
        EventReference::addReference(refs, EventReferenceID::FramePop_ExitPatternEvent, patternEvent);
        EventReference::addReference(refs, EventReferenceID::FramePop_MatchEvent, positiveMatchEvent, negativeMatchEvent);
        return logger->addEvent(SimpleParserEvent::EventID::FramePopedForEarlyExit, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int NodeAdded(EventLogger* logger, QString nodeTypeName, int frameEvent, int patternEvent, const QList<int>& positiveMatchEvent, const QList<int>& negativeMatchEvent, int startPos, int endPos)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::NodeCreation_FrameEvent, frameEvent);
        EventReference::addReference(refs, EventReferenceID::NodeCreation_MatchEvent, patternEvent);
        EventReference::addReference(refs, EventReferenceID::NodeCreation_SupportiveEvent, positiveMatchEvent, negativeMatchEvent);
        QVector<EventLocationRemark> locations = getSingleInputLoc(startPos, endPos);
        return logger->addEvent(SimpleParserEvent::EventID::NodeAdded, QVariantList() << nodeTypeName, EventColorOption::Referable, refs, locations);
    }

    static int MatchPassNoMatching(EventLogger* logger, int frameEvent, int passIndex, int frameIndex, const QList<int>& matchFailEvents)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::MatchPassFail_FrameEvent, frameEvent);
        EventReference::addReference(refs, EventReferenceID::MatchPassFail_MatchEvent, matchFailEvents, QList<int>());
        return logger->addEvent(SimpleParserEvent::EventID::MatchPassNoMatching, QVariantList() << passIndex << frameIndex, EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int MatchFailed(EventLogger* logger, int frameEvent, const QList<int>& passFailEvents)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::MatchFailed_FrameEvent, frameEvent);
        EventReference::addReference(refs, EventReferenceID::MatchFailed_PassFailEvent, passFailEvents, QList<int>());
        return logger->addEvent(SimpleParserEvent::EventID::MatchFailed, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int MatchFinished(EventLogger* logger, int lastFrameEvent, int pos)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::MatchFinished_FrameEvent, lastFrameEvent);
        QVector<EventLocationRemark> locations = getSingleInputLoc(pos, pos);
        return logger->addEvent(SimpleParserEvent::EventID::MatchFinished, QVariantList(), EventColorOption::Referable, refs, locations);
    }

    static int TextToNodePositionMapping(EventLogger* logger, int nodeIndex, int nodeAddEvent)
    {
        EventLocationRemark remark = getOutputLoc(nodeIndex);
        logger->appendEventLocationRemarks(nodeAddEvent, remark);
        return -1;
    }

    static int GarbageAtEnd(EventLogger* logger, int pos, int matchFinishEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PostMatchingCheck_MatchFinishEvent, matchFinishEvent);
        QVector<EventLocationRemark> locations = getSingleInputLoc(pos, pos);
        int code = logger->addEvent(SimpleParserEvent::EventID::GarbageAtEnd, QVariantList(), EventColorOption::Active, refs, locations);
        logger->passFailed(code);
        return code;
    }

    static int PatternMatched(EventLogger* logger, int startPos, int endPos, int sourceEvent, const std::vector<std::pair<int,int>>& elementMatchData)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PatternMatched_SourceEvent, sourceEvent);
        QVector<EventLocationRemark> locations = getSingleInputLoc(startPos, endPos);
        QVariantList data;
        data.reserve(elementMatchData.size());
        for (const auto& patternElementMatchRange : elementMatchData) {
            TextUtil::PlainTextLocation loc(patternElementMatchRange.first, patternElementMatchRange.second);
            QVariant element;
            element.setValue(loc);
            data.push_back(element);
        }
        return logger->addEvent(SimpleParserEvent::EventID::PatternMatched, data, EventColorOption::Referable, refs, locations);
    }

    static int PatternNotMatched(EventLogger* logger, int startPos, int endPos, int sourceEvent, const std::vector<SimpleParser::PatternMatchFailAttempts>& matchAttempts)
    {
        QVariantList data;
        data.reserve(matchAttempts.size());
        for (const auto& attempt : matchAttempts) {
            QVariant element;
            element.setValue(attempt);
            data.push_back(element);
        }
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PatternMatched_SourceEvent, sourceEvent);
        QVector<EventLocationRemark> locations = getSingleInputLoc(startPos, endPos);

        return logger->addEvent(SimpleParserEvent::EventID::PatternNotMatched, data, EventColorOption::Referable, refs, locations);
    }
};

#endif // SIMPLEPARSER_H
