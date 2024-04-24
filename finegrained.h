#include <iostream>
#include <mutex>

class NodeFG {
public:
    int key;
    NodeFG* left;
    NodeFG* right;
    int height;
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

    NodeFG* rightRotate(NodeFG* y);
    NodeFG* leftRotate(NodeFG* x);
    int getBalance(NodeFG* N) const;
    int height(NodeFG* N) const;
    NodeFG* minValueNode(NodeFG* node);

    void insertHelper(NodeFG* node, int key, bool& err);
    NodeFG* deleteHelper(NodeFG* node, int key, bool& err);
    bool searchHelper(NodeFG* node, int key) const;
    void preOrderHelper(NodeFG* node) const;
    void freeTree(NodeFG* node);
};  