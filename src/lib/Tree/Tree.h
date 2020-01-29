#ifndef TREE_H
#define TREE_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>

class TreeBuilder;
class Tree
{
public:
    struct Node {
        QString typeName;
        QStringList keyList;
        QStringList valueList;

        int offsetFromParent; // non-negative; root has offset of zero
        QVector<int> offsetToChildren; // always positive
    };

    struct LocalValueExpression {
        enum ValueType {
            Literal,
            KeyValue
        };
        ValueType ty;
        QString str;
    };

    struct NodeTraverseStep {
        enum class StepDestination {
            Parent,
            Peer,
            Child
        };
        StepDestination destination;
        QString childTypeFilter; // empty for disabled
        QHash<QString, LocalValueExpression> keyValueFilter; // empty for disabled
        int indexDeterminer; // -1 for disabled
        int offsetDeterminer; // 0 for disabled; only used for Peer destination
    };
    struct ValueExpression {
        LocalValueExpression defaultValue;
        // if both traversal and key are empty, default value is directly used
        // reference expression part
        QVector<NodeTraverseStep> traversal;
        QString key;
    };
    struct Predicate {
        enum class PredicateType {
            ValueEqual,
            NodeExist
        };
        PredicateType ty = PredicateType::ValueEqual;
        bool isInvert = false;
        ValueExpression lhs;
        ValueExpression rhs;
        QVector<NodeTraverseStep> nodeExpr;
    };

public:

    Tree() = default;
    Tree(const Tree&) = default;
    Tree(Tree&&) = default;
    Tree(const TreeBuilder& tree);
    ~Tree() = default;

    // used in executing
    int nodeTraverse(int startNodeIndex, const NodeTraverseStep& step);// -1 if failed
    QString evaluateValueReferenceExpression(int startNodeIndex, const ValueExpression& expr, bool* isGood = nullptr);
    bool evaluatePredicate(int startNodeIndex, const Predicate& pred, bool* isGood = nullptr);


public:
    const Node& getNode(int index) const {return nodes.at(index);}
    int getNumNodes() const {return nodes.size();}
    bool isEmpty() const {return nodes.isEmpty();}

private:
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
    private:
        Node* parent = nullptr;
        Node* childStart = nullptr;
        // a doubly linked list among peers in the same hierarchy
        Node* previousPeer = nullptr;
        Node* nextPeer = nullptr;
    };
    ~TreeBuilder(){clear();}
    void clear();

    void swap(TreeBuilder& rhs) {
        nodes.swap(rhs.nodes);
    }

    void setRoot(Node* newRoot){root = newRoot;}
    Node* addNode();
    Node* addNode(Node* parent);
private:
    static void populateNodeList(QVector<Tree::Node>& nodes, QVector<int>& childTable, int parentIndex, TreeBuilder::Node* subtreeRoot);
private:
    Node* root = nullptr;
    QList<Node*> nodes;
};

#endif // TREE_H
