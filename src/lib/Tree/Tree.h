#ifndef TREE_H
#define TREE_H

#include <QString>
#include <QStringRef>
#include <QStringList>
#include <QList>
#include <QVector>
#include <QHash>
#include <QMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "src/utils/XMLUtilities.h"

class TreeBuilder;
class Tree
{
public:
    struct Node {
        QString typeName;
        QStringList keyList;
        QStringList valueList;

        int offsetFromParent = 0; // non-negative; root has offset of zero
        QVector<int> offsetToChildren; // always positive
    };

    struct LocalValueExpression {
        enum ValueType {
            Literal,
            KeyValue,
            NodeType
        };
        ValueType ty = ValueType::Literal;
        QString str;

        void saveToXML(QXmlStreamWriter& xml) const; // caller should make sure that the StartElement is written
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    struct NodeTraverseStep {
        enum class StepDestination {
            Parent,
            Peer,
            Child
        };
        StepDestination destination = StepDestination::Parent;
        QString childTypeFilter; // empty for disabled
        QMap<QString, LocalValueExpression> keyValueFilter; // empty for disabled
        int indexDeterminer = -1; // -1 for disabled
        int offsetDeterminer = 0; // 0 for disabled; only used for Peer destination

        void saveToXML(QXmlStreamWriter& xml) const; // caller should make sure that the StartElement is written
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);

        static void saveToXML(QXmlStreamWriter& xml, const QVector<NodeTraverseStep>& steps); // caller should make sure that the StartElement is written
        static bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache, QVector<NodeTraverseStep>& steps);

        static void saveToXML(QXmlStreamWriter& xml, const QVector<NodeTraverseStep>& steps, int treeIndex); // caller should make sure that the StartElement is written
        static bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache, QVector<NodeTraverseStep>& steps, int& treeIndex);
    };

    struct EvaluationContext {
        const Tree& mainTree;
        QList<const Tree*> sideTreeList;
        int startNodeIndex = 0;
        explicit EvaluationContext(const Tree& mainTreeRef)
            : mainTree(mainTreeRef)
        {}
        explicit EvaluationContext(const Tree& mainTreeRef, const QList<const Tree*>& sideTreeListRef, int startNode)
            : mainTree(mainTreeRef), sideTreeList(sideTreeListRef), startNodeIndex(startNode)
        {}
        EvaluationContext(const EvaluationContext&) = default;
        EvaluationContext(EvaluationContext&&) = default;
    };

    struct SingleValueExpression {
        enum class EvaluateStrategy {
            DefaultOnly,
            TraverseOnly,
            TraverseWithFallback
        };
        int treeIndex = -1;
        EvaluateStrategy es = EvaluateStrategy::DefaultOnly;
        LocalValueExpression defaultValue;
        // if both traversal and key are empty, default value is directly used
        // reference expression part
        QVector<NodeTraverseStep> traversal;
        LocalValueExpression exprAtDestinationNode;

        void saveToXML(QXmlStreamWriter& xml) const; // caller should make sure that the StartElement is written
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };
    struct Predicate {
        enum class PredicateType {
            ValueEqual, // v1: lhs, v2: rhs
            NodeExist
        };
        PredicateType ty = PredicateType::ValueEqual;
        bool isInvert = false;

        // two values, depending on predicate type
        SingleValueExpression v1;
        SingleValueExpression v2;

        struct NodeTestData {
            QVector<NodeTraverseStep> steps;
            int treeIndex = -1;
        } nodeTest;

        void saveToXML(QXmlStreamWriter& xml) const; // caller should make sure that the StartElement is written
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);

        static void saveToXML(QXmlStreamWriter& xml, const QVector<Predicate>& predicates); // caller should make sure that the StartElement is written
        static bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache, QVector<Predicate>& predicates);
    };

    struct BranchedValueExpression {
        struct Branch {
            QVector<Predicate> predicates;
            SingleValueExpression value;

            void saveToXML(QXmlStreamWriter& xml) const; // caller should make sure that the StartElement is written
            bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
        };
        QVector<Branch> branches;

        void saveToXML(QXmlStreamWriter& xml) const; // caller should make sure that the StartElement is written
        bool loadFromXML(QXmlStreamReader& xml, StringCache& strCache);
    };

    // nah the user will probably use scripting if more complex things are needed..
    using GeneralValueExpression = BranchedValueExpression;

public:

    Tree() = default;
    Tree(const Tree&) = default;
    Tree(Tree&&) = default;
    Tree& operator=(const Tree&) = default;

    Tree(const TreeBuilder& tree);
    Tree(const TreeBuilder& tree, QVector<int>& sequenceNumberTable);
    ~Tree() = default;
    void swap(Tree& rhs) {
        nodes.swap(rhs.nodes);
    }

    // used in executing

    static QString evaluateLocalValueExpression(const Node &startNode, const LocalValueExpression& expr, bool& isGood);
    /**
     * @brief nodeTraverse traverse one step from current node; return the result node index
     * @param currentNodeIndex the node index where the traverse is made
     * @param localValueEvaluationNode the node where LocalValueExpression's KeyValue will be evaluated (not necessarily in the same tree)
     * @param step struct that describes the node traverse step
     * @param isGood whether the step is successful; the return value must not be used if isGood is false
     * @return the result node index; undefined if isGood is false
     */
    int nodeTraverse(int currentNodeIndex, const Node& localValueEvaluationNode, const NodeTraverseStep& step, bool& isGood) const;

    int nodeTraverse(int currentNodeIndex, const Node& localValueEvaluationNode, const QVector<NodeTraverseStep>& steps, bool& isGood) const;

    static int nodeTraverse(const QVector<NodeTraverseStep> &steps, bool &isGood, const EvaluationContext& ctx, int treeIndex);
    static QString evaluateSingleValueExpression(const SingleValueExpression& expr, bool& isGood, const EvaluationContext& ctx);
    static bool evaluatePredicate(const Predicate& pred, bool& isGood, const EvaluationContext& ctx);
    static QString evaluateGeneralValueExpression(const GeneralValueExpression& expr, bool& isGood, const EvaluationContext& ctx);


public:
    const Node& getNode(int index) const {return nodes.at(index);}
    int getNumNodes() const {return nodes.size();}
    bool isEmpty() const {return nodes.isEmpty();}

protected:
    QVector<Node> nodes;
};

class TreeBuilder
{
    friend class Tree;
public:
    struct Node {
        friend class TreeBuilder;
    public:
        QString typeName;
        QStringList keyList;
        QStringList valueList;

        /**
         * @brief detach removes the node from the tree
         *
         * This function does nothing for nodes already detached from the tree.
         * No memory deallocation is made and the node can be inserted back any time.
         */
        void detach();

        /**
         * @brief setParent detach the node from current location and insert back at tail of child list of new parent
         * @param newParent the new parent for this node
         */
        void setParent(Node* newParent);

        /**
         * @brief changePosition adjust the node from current location
         *
         * The node is detached first, then inserted to newParent with newPredecessor
         * If newParent is nullptr, then the current parent is used
         * If newPredecessor is nullptr, then the node will be inserted at the beginning of child list
         *
         * @param newParent the new parent for this node; null to stay under current parent node
         * @param newPredecessor the new predecessor node for this node; null to insert before any peers
         */
        void changePosition(Node *newParent, Node* newPredecessor);

        void setDataFromNode(const Tree::Node& src) {
            typeName = src.typeName;
            keyList = src.keyList;
            valueList = src.valueList;
        }

        int getSequenceNumber() const {
            return sequenceNumber;
        }
    private:
        Node* parent = nullptr;
        Node* childStart = nullptr;
        // a doubly linked list among peers in the same hierarchy
        Node* previousPeer = nullptr;
        Node* nextPeer = nullptr;
        int sequenceNumber = 0;
    };
    ~TreeBuilder(){clear();}
    void clear();

    void swap(TreeBuilder& rhs) {
        nodes.swap(rhs.nodes);
    }

    void setRoot(Node* newRoot){root = newRoot;}
    Node* allocateNode();
    Node* addNode(Node* parent);
private:
    static void populateNodeList(QVector<Tree::Node>& nodes, QVector<int>& childTable, QVector<int>* sequenceNumberTable, int parentIndex, TreeBuilder::Node* subtreeRoot);
private:
    Node* root = nullptr;
    QList<Node*> nodes;
    int sequenceCounter = 0;
};

#endif // TREE_H
