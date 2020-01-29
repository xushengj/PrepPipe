#ifndef SIMPLETREETRANSFORM_H
#define SIMPLETREETRANSFORM_H

#include "src/lib/Tree/Tree.h"

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
class SimpleTreeTransform
{
public:
    struct SubTreeNodeTemplate {
        Tree::ValueExpression ty;
        QVector<Tree::ValueExpression> keyList;
        QVector<Tree::ValueExpression> valueList;
        QVector<int> childTemplateIndexList;
    };
    struct NodeTransformPattern {
        enum TransformFlag {
            None = 0x00,
            PassThroughRemainingKeyValues = 0x01
        };
        Q_DECLARE_FLAGS(TransformFlags, TransformFlag)

        TransformFlags flags;
        QVector<Tree::Predicate> predicates;
        QVector<int> rootLevelNodeTemplates;
        QVector<QVector<Tree::NodeTraverseStep>> skipNodes;
    };
private:
    QVector<SubTreeNodeTemplate> nodeTemplates; // index -1 for pass-through
    QHash<QString, QVector<NodeTransformPattern>> nodeTypeToRuleList;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SimpleTreeTransform::NodeTransformPattern::TransformFlags)

#endif // SIMPLETREETRANSFORM_H
