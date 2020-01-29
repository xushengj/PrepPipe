#include "src/lib/Tree/Tree.h"

#include <stdexcept>

int Tree::nodeTraverse(int startNodeIndex, const NodeTraverseStep& step)
{
    Q_UNUSED(startNodeIndex)
    Q_UNUSED(step)
    qFatal("Unimplemented!");
    return 0;
}

QString Tree::evaluateValueReferenceExpression(int startNodeIndex, const ValueExpression& expr, bool* isGood)
{
    Q_UNUSED(startNodeIndex)
    Q_UNUSED(expr)
    Q_UNUSED(isGood)
    qFatal("Unimplemented!");
    return QString();
}

bool Tree::evaluatePredicate(int startNodeIndex, const Predicate& pred, bool* isGood)
{
    Q_UNUSED(startNodeIndex)
    Q_UNUSED(pred)
    Q_UNUSED(isGood)
    qFatal("Unimplemented!");
    return false;
}

// ----------------------------------------------------------------------------

Tree::Tree(const TreeBuilder &tree)
{
    Q_ASSERT(tree.root != nullptr);
    QVector<int> dummy;
    TreeBuilder::populateNodeList(nodes, dummy, 0, tree.root);
    Q_ASSERT(dummy.size() == 1);
}

void TreeBuilder::populateNodeList(QVector<Tree::Node>& nodes, QVector<int>& childTable, int parentIndex, TreeBuilder::Node* subtreeRoot)
{
    Q_ASSERT(subtreeRoot);
    int nodeIndex = nodes.size();
    {
        Tree::Node curNode;
        int offset = nodeIndex - parentIndex;
        curNode.offsetFromParent = offset;
        curNode.typeName = subtreeRoot->typeName;
        curNode.keyList = subtreeRoot->keyList;
        curNode.valueList = subtreeRoot->valueList;
        nodes.push_back(curNode);
        childTable.push_back(offset);
    }
    QVector<int> childOffsets;
    for (TreeBuilder::Node* child = subtreeRoot->childStart; child != nullptr; child = child->nextPeer) {
        populateNodeList(nodes, childOffsets, nodeIndex, child);
    }
    nodes[nodeIndex].offsetToChildren.swap(childOffsets);
}

void TreeBuilder::Node::detach()
{
    // if the node is already detached, leave it there
    if (parent == nullptr) {
        Q_ASSERT(previousPeer == nullptr && nextPeer == nullptr);
        return;
    }

    if (previousPeer) {
        Q_ASSERT(previousPeer->nextPeer == this);
        previousPeer->nextPeer = nextPeer;
    } else {
        Q_ASSERT(parent->childStart == this);
        parent->childStart = nextPeer;
    }

    if (nextPeer) {
        Q_ASSERT(nextPeer->previousPeer == this);
        nextPeer->previousPeer = previousPeer;
    }

    parent = nullptr;
    previousPeer = nullptr;
    nextPeer = nullptr;
}

void TreeBuilder::Node::setParent(Node* newParent)
{
    Q_ASSERT(newParent);

    detach();
    parent = newParent;
    if (newParent->childStart == nullptr) {
        newParent->childStart = this;
        return;
    }
    Node* predecessor = newParent->childStart;
    while (predecessor->nextPeer != nullptr) {
        predecessor = predecessor->nextPeer;
    }
    predecessor->nextPeer = this;
    previousPeer = predecessor;
}

void TreeBuilder::Node::changePosition(Node* newParent, Node* newPredecessor)
{
    if (newParent == nullptr)
        newParent = parent;

    Q_ASSERT(newParent);

    detach();
    parent = newParent;
    if (newParent->childStart == nullptr) {
        Q_ASSERT(newPredecessor == nullptr);
        newParent->childStart = this;
        return;
    }

    Node* predecessor = newParent->childStart;
    while (predecessor != newPredecessor) {
        predecessor = predecessor->nextPeer;
        Q_ASSERT(predecessor);
    }
    predecessor->nextPeer = this;
    previousPeer = predecessor;
}

void TreeBuilder::clear(){
    for (auto ptr : nodes) {
        if (ptr)
            delete ptr;
    }
}

TreeBuilder::Node* TreeBuilder::addNode()
{
    Node* ptr = new Node;
    if (!root)
        root = ptr;
    nodes.push_back(ptr);
    return ptr;
}

TreeBuilder::Node* TreeBuilder::addNode(Node* parent)
{
    Node* ptr = new Node;
    nodes.push_back(ptr);
    ptr->setParent(parent);
    return ptr;
}
