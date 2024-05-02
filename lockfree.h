#include <iostream>
#include <vector>
#include <mutex>

class NodeLF {
public:
    volatile int key;
    volatile NodeLF* left;
    volatile NodeLF* right;

    NodeLF(int key);
};

class InsertOp : public Operation {
public:
    bool isLeft;
    bool isUpdate = false;
    NodeLF* expectedNode;
    NodeLF* newNode;

    InsertOp(bool isLeft, bool isUpdate, NodeLF* expected, NodeLF* new);
};

class RotateOp : public Operation {
public:
    volatile int state = 0;
    NodeLF* node;
    Operation* nodeOp;
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
