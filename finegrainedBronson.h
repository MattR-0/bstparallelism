/* Reference: https://stanford-ppl.github.io/website/papers/ppopp207-bronson.pdf */
#include <iostream>
#include <mutex>

class NodeFG {
public:
    enum NodeType {INT, REM};
    long version;
    int height;
    const int key;
    NodeType value; // determinant for removed node
    
    NodeFG* left;
    NodeFG* right;
    NodeFG* parent;

    std::mutex nodeLock;

    NodeFG(int key);
};

class AVLTreeFG {
public:
    // NodeFG* root;
    AVLTreeFG();
    ~AVLTreeFG();

    bool insert(int key);
    bool deleteNode(int key);
    bool search(int key);
    void preOrder();

private:
    std::mutex rootLock;
    NodeFG* rootHolder;
    // Specify the rollback of optimistic concurrency control
    enum Status {RETRY, SUCCESS, FAILURE};

    int getBalance(NodeFG* node) const;
    int height(NodeFG* node) const;
    NodeFG* minValueNode(NodeFG* node);
    NodeFG* getChild(NodeFG* node, int dir);
    bool canUnlink(NodeFG* node);
    void waitUntilNotChanging(NodeFG* node);

    int nodeCondition(NodeFG* node);
    NodeFG* fixHeightNoLock(NodeFG* node);
    void fixHeightAndRebalance(NodeFG* node);
    NodeFG* rotateRight(NodeFG* parent, NodeFG* node, NodeFG* nL, int hR, int hLL, NodeFG* nLR, int hLR);
    NodeFG* rotateLeft(NodeFG* parent, NodeFG* node, NodeFG* nR, int hL, int hRR, NodeFG* nRL, int hRL);
    NodeFG* rotateRightOverLeft(NodeFG* parent, NodeFG* node, NodeFG* nL, int hR, int hLL, NodeFG* nLR, int hLRL);
    NodeFG* rotateLeftOverRight(NodeFG* parent, NodeFG* node, NodeFG* nR, int hL, int hRR, NodeFG* nRL, int hRLR);
    NodeFG* rebalanceToRight(NodeFG* parent, NodeFG* node, NodeFG* nL, int hR0);
    NodeFG* rebalanceToLeft(NodeFG* parent, NodeFG* node, NodeFG* nR, int hL0);
    NodeFG* rebalanceNoLock(NodeFG* parent, NodeFG* node);

    Status attemptInsert(int key, NodeFG* node, int dir, long nodeV);
    Status attemptInsertHelper(int key, NodeFG* node, int dir, long nodeV);
    Status attemptDeleteNode(int key, NodeFG* node, int dir, long nodeV);
    Status attemptRemoveNode(NodeFG* parent, NodeFG* node);
    bool attemptUnlinkNoLock(NodeFG* parent, NodeFG* node);
    Status attemptSearch(int key, NodeFG* node, int dir, long nodeV);    
    
    void preOrderHelper(NodeFG* node) const;
    void freeTree(volatile NodeFG* node);
};  