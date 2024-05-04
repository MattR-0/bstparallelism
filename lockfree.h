#include <iostream>
#include <vector>
#include <mutex>

class Operation {};

class NodeLF {
public:
    volatile int key;
    volatile NodeLF* left;
    volatile NodeLF* right;
    volatile int height;
    volatile int lh;
    volatile int rh;
    volatile Operation* op;
    volatile bool deleted;
    volatile bool removed;

    NodeLF(int key);
};

class InsertOp : public Operation {
public:
    bool isLeft;
    NodeLF* expectedNode;
    NodeLF* newNode;

    InsertOp(bool isLeft, NodeLF* expected, NodeLF* new);
};

class RotateOp : public Operation {
public:
    volatile int state = 0;
    NodeLF* parent;
    NodeLF* node;
    NodeLF* child;
    Operation* parentOp;
    Operation* nodeOp;
    Operation* childOp;
    bool rightR;
    bool dir;
    int oldKey;
    int newKey;

    RotateOp(NodeLF* node, Operation* nodeOp, int oldKey, int newKey);
}

class AVLTreeLF {
public:
    NodeLF* root;
    AVLTreeLF();
    ~AVLTreeLF();

    bool insert(int key);
    bool deleteNode(int key);
    bool search(int key);

private:
    NodeLF* rightRotate(NodeLF* y);
    NodeLF* leftRotate(NodeLF* x);
    int getBalance(NodeLF* N) const;
    int height(NodeLF* N) const;
    NodeLF* minValueNode(NodeLF* node);

    NodeLF* insertHelper(NodeLF* node, int key, bool& err);
    NodeLF* deleteHelper(NodeLF* node, int key, bool& err);
    bool searchHelper(NodeLF* node, int key) const;
    void freeTree(NodeLF* node);

    bool compareAndSwap(NodeLF* node1, NodeLF* node2);
};
