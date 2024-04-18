#include <iostream>
#include <vector>
#include <mutex>

class NodeCG {
public:
    int key;
    NodeCG* left;
    NodeCG* right;
    int height;

    NodeCG(int key);
};

class AVLTreeCG {
public:
    NodeCG* root;
    AVLTreeCG();
    ~AVLTreeCG();

    bool insert(int key);
    bool deleteNode(int key);
    bool search(int key);
    void preOrder();

private:
    std::mutex writeLock;
    std::mutex readLock;
    int readCount;

    void startWrite();
    void endWrite();
    void startRead();
    void endRead();
    
    NodeCG* rightRotate(NodeCG* y);
    NodeCG* leftRotate(NodeCG* x);
    int getBalance(NodeCG* N) const;
    int height(NodeCG* N) const;
    NodeCG* minValueNode(NodeCG* node);

    NodeCG* insertHelper(NodeCG* node, int key, bool& err);
    NodeCG* deleteHelper(NodeCG* node, int key, bool& err);
    bool searchHelper(NodeCG* node, int key) const;
    void preOrderHelper(NodeCG* node) const;
    void freeTree(NodeCG* node);
};

