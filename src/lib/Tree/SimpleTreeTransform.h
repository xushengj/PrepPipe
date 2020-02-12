#ifndef SIMPLETREETRANSFORM_H
#define SIMPLETREETRANSFORM_H

#include "src/lib/Tree/Tree.h"

// WARNING: outdated comment
// a tree to tree transform has following parts:
//   1. a default policy for unknown tree node types (keep or drop) and unspecified parameters (keep or drop)
//   2. a set of node transform rules (unordered)
// a transform rule should have following parts:
//   1. a node type to be applied on
//   2. an ordered list of transform patterns
// a transform pattern should have following parts:
//   1. a set of predicates that must all be satisfied for the pattern to be applied (can be empty for default pattern)
//      (likely to be extended frequently in the future; maybe just "value reference expr" == literal for now?)
//   2. a set of node definition expression for what to put under insertion point of new tree
//   3. a set of node reference expression for what (child or successor) node to skip in the following processing

#include "src/utils/XMLUtilities.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <functional>

class SimpleTreeTransform
{
public:
    struct KeyValueExpressionPair {
        Tree::GeneralValueExpression key;
        Tree::GeneralValueExpression value;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };
    struct SubTreeNodeTemplate {
        // if both ty and key-value list are empty, the node will copy data from current node
        QVector<KeyValueExpressionPair> kvList;
        Tree::GeneralValueExpression ty;
        int parentIndex = -1; // -1 for one of the node in root level

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };
    enum class PredefinedAction {
        PassThrough,
        Remove,
        Error
    };

    struct NodeTransformRule {
        enum class TransformType {
            PassThrough, // subtree will be checked
            Remove, // remove entire subtree starting from this node
            Replace, // replace entire subtree; subtree will not be checked
            Modify // modify current node; subtree will be checked
        };

        TransformType ty = TransformType::PassThrough;
        QVector<Tree::Predicate> predicates;
        QVector<SubTreeNodeTemplate> nodeTemplates; // only for replace
        QVector<KeyValueExpressionPair> modifications; // only for modify; empty key for modifying type, empty value for removal
        QVector<QVector<Tree::NodeTraverseStep>> skipNodes; // a list of nodes who would be skipped (removed)

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct Data {
        PredefinedAction defaultAction = PredefinedAction::PassThrough;
        PredefinedAction unrecognizedNodeActionOverride = PredefinedAction::PassThrough;
        bool isUnrecognizedNodeUseDefaultAction = true;

        QHash<QString, QVector<NodeTransformRule>> nodeTypeToRuleList;
        QStringList sideTreeNameList;

        void saveToXML(QXmlStreamWriter& xml) const;
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct NodeProvenance {
        int srcNodeIndex = -1;
        int patternIndex = -1; // -1 for default action
    };

    struct TransformError {
        enum class Cause {
            RequestedError_UnrecognizedNodeType,
            RequestedError_DefaultActionForNoRuleMatch,
            EvaluationFail_NodeTemplate_NodeType,
            EvaluationFail_NodeTemplate_KeyValue,
            EvaluationFail_Modification
        };
        struct KeyValueEvaluationFailData {
            QString key;
            QString value;
            int keyValueIndex = -1;
            bool isKeyGood = false;
            bool isValueGood = false;
        };
        int srcNodeIndex = -1;
        int patternIndex = -1;

        Cause cause;
        KeyValueEvaluationFailData evalFailData;
        QString unrecognizedNodeType;
        int modificationIndex = -1;
        // TODO enrich this
    };

public:
    explicit SimpleTreeTransform(const Data& d)
        : data(d)
    {}
    bool performTransform(const Tree& tree, Tree& dest, const QList<const Tree*>& sideTreeList) const;
private:
    Data data;
    // All data should be in Data; do not add stuff here
};

#endif // SIMPLETREETRANSFORM_H
