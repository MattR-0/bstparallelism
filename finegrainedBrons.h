/* Reference: https://stanford-ppl.github.io/website/papers/ppopp207-bronson.pdf */
#include <iostream>
#include <mutex>

    class NodeFG {
    public:
        enum NodeType {INT, ROU};
        volatile long version;
        volatile int height;
        const int key;
        NodeType value; // determinant for routing node
        
        volatile NodeFG* left;
        volatile NodeFG* right;
        volatile NodeFG* parent;

        std::mutex nodeLock;

        NodeFG(int key);
    };

class AVLTreeFG {
public:
    NodeFG* root;
    AVLTreeFG();
    ~AVLTreeFG();

    bool insert(int key);
    bool deleteNode(int key);
    bool search(int key);
    void preOrder();

private:
    std::mutex rootLock;
    NodeFG* rootHolder;

    void rightRotate(NodeFG* node);
    void leftRotate(NodeFG* node);
    int getBalance(NodeFG* node) const;
    int height(NodeFG* node) const;
    NodeFG* minValueNode(NodeFG* node);
    void fixHeightAndRebalance(NodeFG* node);
    NodeFG* getChild(NodeFG* node, int dir);
    bool canUnlink(NodeFG* node);
    void waitUntilNotChanging(NodeFG* node);

    Status attempInsert(int key, NodeFG* node, int dir, long nodeV);
    Status attempInsertHelper(int key, NodeFG* node, int dir, long nodeV);
    Status attemptDeleteNode(int key, NodeFG* node, int dir, long nodeV);
    Status attemptRemoveNode(NodeFG* parent, NodeFG* node);
    Status attemptSearch(int key, NodeFG* node, int dir, long nodeV);    
    
    void preOrderHelper(NodeFG* node) const;
    void freeTree(NodeFG* node);
};  