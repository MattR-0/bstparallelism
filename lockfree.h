#include <iostream>
#include <vector>
#include <mutex>

class NodeLF {
public:
    int key;
    volatile int value;
    volatile NodeLF* parent;
    volatile NodeLF* left;
    volatile NodeLF* right;
    volatile int height;
    volatile long version; // Bottom 3 bits: unlink, growing change, shrinking change

    NodeLF(int key);
    NodeLF* child(int direction);
};

class AVLTreeLF {
public:
    NodeLF* rootHolder; // empty node w/ root as right child
    AVLTreeLF();
    ~AVLTreeLF();

    bool insert(int key);
    bool deleteNode(int key);
    bool search(int key);

private:
    std::atomic<int> x;
    
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
