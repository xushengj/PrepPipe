#ifndef SIMPLEPARSER_H
#define SIMPLEPARSER_H

#include "src/lib/Tree/Tree.h"
#include "src/lib/Tree/EventLogging.h"
#include "src/utils/XMLUtilities.h"

#include <QString>
#include <QStringList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QRegularExpression>

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

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct MatchRuleNode {
        QString name;
        QStringList parentNodeNameList;
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
        QString rootRuleNodeName;
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
        int strLength = 0;
        int curPosition = 0;

        void clear();
        void set(const QString& text, EventLogger* loggerArg);
        int getRegexIndex(const QString& pattern);
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

    PatternMatchResult tryPattern(const Pattern& pattern, int pos, int patternTestSourceEvent);

private:
    // "volatile" data that can be recomputed or ones that are just cache

    // derived helper data
    QHash<QString, int> boundaryNameToIndexMap;
    QHash<QString, int> contentTypeNameToIndexMap;
    QHash<int, QVector<int>> classBasedBoundaryChildList;
    QVector<QVector<int>> childNodeMatchRuleVec; // NodeMatchRuleIndex -> vec of child node match rule index
    int rootNodeRuleIndex = -1;
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

class SimpleParserEvent : public EventInterpreter
{
public:
    enum class EventID: int {
        RootNodeSpecification,
        RootNodeDirectCreation,
        RootNodePatternMatchCreation,
        RootNodePatternMatchFailed,
        EmptyLineSkipped,
        BetterPatternMatched,
        PatternMatchedNotBetter,
        FramePushed,
        FramePoped,
        FramePopedForEarlyExit,
        RootNodeNoFrame,
        NodeAdded,
        MatchFinished,
        TextToNodePositionMapping,
        GarbageAtEnd,
        PatternMatched,
        PatternNotMatched
    };
    enum class EventLocationID: int {
        START = static_cast<int>(EventLocationType::OTHER_START),
        END
    };
    enum class EventReferenceID: int {
        RootNodeSpecification,
        RootNodeCreationEvent,
        PatternMatch_MatchFinalEvent,
        PatternMatch_SupportiveEvent, // both positive and negative
        PatternBestCandidateComparison_PreviousEvent,
        PatternBestCandidateComparison_NewEvent,
        FramePush_ParentNodeCreationEvent,
        FramePop_PushEvent,
        FramePop_ExitPatternEvent,
        FramePop_MatchEvent,
        NodeCreation_FrameEvent,
        NodeCreation_MatchEvent,
        NodeCreation_SupportiveEvent,
        MatchFinished_FrameEvent,
        PositionMapping_NodeEvent,
        PostMatchingCheck_MatchFinishEvent,
        PatternMatched_SourceEvent,
        PatternMatched_ElementMatchEvent,
        PatternNotMatched_SourceEvent,
        PatternNotMatched_ElementMatchEvent
    };
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
    static int RootNodeSpecification(EventLogger* logger, int rootNodeIndex, const QString& rootNodeName)
    {
        return logger->addEvent(SimpleParserEvent::EventID::RootNodeSpecification,
                                QVariantList() << rootNodeIndex << rootNodeName);
    }

    static int RootNodeDirectCreation(EventLogger* logger, int rootNodeSpecificationEvent)
    {
        return logger->addEvent(SimpleParserEvent::EventID::RootNodeDirectCreation,
                                QVariantList() << rootNodeSpecificationEvent);
    }

    static int RootNodePatternMatchCreation(EventLogger* logger, int rootNodePatternEvent, const QList<int>& positiveMatchEvent, const QList<int>& negativeMatchEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PatternMatch_MatchFinalEvent, rootNodePatternEvent);
        EventReference::addReference(refs, EventReferenceID::PatternMatch_SupportiveEvent, positiveMatchEvent, negativeMatchEvent);
        return logger->addEvent(SimpleParserEvent::EventID::RootNodePatternMatchCreation, QVariantList(), EventColorOption::Referable, refs);
    }

    static int RootNodePatternMatchFailed(EventLogger* logger, int failedPos, int rootNodeSpecificationEvent, const QList<int>& positiveMatchEvent, const QList<int>& negativeMatchEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::RootNodeSpecification, rootNodeSpecificationEvent);
        EventReference::addReference(refs, EventReferenceID::PatternMatch_SupportiveEvent, positiveMatchEvent, negativeMatchEvent);
        QVector<EventLocationRemark> locations;
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, failedPos);
        return logger->addEvent(SimpleParserEvent::EventID::RootNodePatternMatchFailed, QVariantList(), EventColorOption::Referable, refs, locations);
    }

    static int EmptyLineSkipped(EventLogger* logger, int startPos, int endPos)
    {
        QVector<EventLocationRemark> locations;
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, startPos);
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataEnd, endPos);
        return logger->addEvent(SimpleParserEvent::EventID::EmptyLineSkipped, QVariantList(), EventColorOption::Passive, QVector<EventReference>(), locations);
    }

    static int BetterPatternMatched(EventLogger* logger, int betterPatternEvent, int prevPatternEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PatternBestCandidateComparison_NewEvent, betterPatternEvent);
        EventReference::addReference(refs, EventReferenceID::PatternBestCandidateComparison_PreviousEvent, prevPatternEvent);
        return logger->addEvent(SimpleParserEvent::EventID::BetterPatternMatched, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int PatternMatchedNotBetter(EventLogger* logger, int newPatternEvent, int prevBetterPatternEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PatternBestCandidateComparison_NewEvent, newPatternEvent);
        EventReference::addReference(refs, EventReferenceID::PatternBestCandidateComparison_PreviousEvent, prevBetterPatternEvent);
        return logger->addEvent(SimpleParserEvent::EventID::PatternMatchedNotBetter, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int FramePushed(EventLogger* logger, int parentNodeAddEvent, const QVector<int>& childRuleNodeIndices)
    {
        QVariantList data;
        for (int idx : childRuleNodeIndices) {
            data.push_back(idx);
        }
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::FramePush_ParentNodeCreationEvent, parentNodeAddEvent);
        return logger->addEvent(SimpleParserEvent::EventID::FramePushed, data, EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int FramePoped(EventLogger* logger, int frameEvent, const QList<int>& matchFailEvents)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::FramePop_PushEvent, frameEvent);
        EventReference::addReference(refs, EventReferenceID::FramePop_MatchEvent, QList<int>(), matchFailEvents);
        return logger->addEvent(SimpleParserEvent::EventID::FramePoped, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int FramePopedForEarlyExit(EventLogger* logger, int frameEvent, int patternEvent, const QList<int>& positiveMatchEvent, const QList<int>& negativeMatchEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::FramePop_PushEvent, frameEvent);
        EventReference::addReference(refs, EventReferenceID::FramePop_ExitPatternEvent, patternEvent);
        EventReference::addReference(refs, EventReferenceID::FramePop_MatchEvent, positiveMatchEvent, negativeMatchEvent);
        return logger->addEvent(SimpleParserEvent::EventID::FramePopedForEarlyExit, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int RootNodeNoFrame(EventLogger* logger, int parentNodeEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::RootNodeCreationEvent, parentNodeEvent);
        return logger->addEvent(SimpleParserEvent::EventID::RootNodeNoFrame, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int NodeAdded(EventLogger* logger, int frameEvent, int patternEvent, const QList<int>& positiveMatchEvent, const QList<int>& negativeMatchEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::NodeCreation_FrameEvent, frameEvent);
        EventReference::addReference(refs, EventReferenceID::NodeCreation_MatchEvent, patternEvent);
        EventReference::addReference(refs, EventReferenceID::NodeCreation_SupportiveEvent, positiveMatchEvent, negativeMatchEvent);
        return logger->addEvent(SimpleParserEvent::EventID::NodeAdded, QVariantList(), EventColorOption::Referable, refs, QVector<EventLocationRemark>());
    }

    static int MatchFinished(EventLogger* logger, int lastFrameEvent, int pos)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::MatchFinished_FrameEvent, lastFrameEvent);
        QVector<EventLocationRemark> locations;
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, pos);
        return logger->addEvent(SimpleParserEvent::EventID::MatchFinished, QVariantList(), EventColorOption::Referable, refs, locations);
    }

    static int TextToNodePositionMapping(EventLogger* logger, int startPos, int endPos, int nodeIndex, int nodeAddEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PositionMapping_NodeEvent, nodeAddEvent);
        QVector<EventLocationRemark> locations;
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, startPos);
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataEnd, endPos);
        EventLocationRemark::addRemark(locations, EventLocationType::OutputDataStart, nodeIndex);
        return logger->addEvent(SimpleParserEvent::EventID::TextToNodePositionMapping, QVariantList(), EventColorOption::Active, refs, locations);
    }

    static int GarbageAtEnd(EventLogger* logger, int pos, int matchFinishEvent)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PostMatchingCheck_MatchFinishEvent, matchFinishEvent);
        QVector<EventLocationRemark> locations;
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, pos);
        return logger->addEvent(SimpleParserEvent::EventID::GarbageAtEnd, QVariantList(), EventColorOption::Active, refs, locations);
    }

    static int PatternMatched(EventLogger* logger, int startPos, int endPos, int sourceEvent, const QList<int>& elementMatchEvents)
    {
        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PatternMatched_ElementMatchEvent, elementMatchEvents, QList<int>());
        EventReference::addReference(refs, EventReferenceID::PatternMatched_SourceEvent, sourceEvent);
        QVector<EventLocationRemark> locations;
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, startPos);
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, endPos);
        return logger->addEvent(SimpleParserEvent::EventID::PatternMatched, QVariantList(), EventColorOption::Referable, refs, locations);
    }

    static int PatternNotMatched(EventLogger* logger, int startPos, int endPos, int sourceEvent, int elementIndex, const SimpleParser::PatternElement& element, const QList<int>& elementMatchEvents)
    {
        QVariantList data;
        data << elementIndex;
        data << static_cast<int>(element.ty);
        data << element.str;

        QVector<EventReference> refs;
        EventReference::addReference(refs, EventReferenceID::PatternMatched_ElementMatchEvent, elementMatchEvents, QList<int>());
        EventReference::addReference(refs, EventReferenceID::PatternMatched_SourceEvent, sourceEvent);
        QVector<EventLocationRemark> locations;
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, startPos);
        EventLocationRemark::addRemark(locations, EventLocationType::InputDataStart, endPos);
        return logger->addEvent(SimpleParserEvent::EventID::PatternNotMatched, data, EventColorOption::Referable, refs, locations);
    }
};

#endif // SIMPLEPARSER_H
