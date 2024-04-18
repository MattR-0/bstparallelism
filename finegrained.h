#include <iostream>
#include <vector>
#include <mutex>

class NodeFG {
public:
    int key;
    NodeFG* left;
    NodeFG* right;
    int height;
    std::mutex lock;

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

private:
    std::mutex lock;

    Node* rightRotate(Node* y);
    Node* leftRotate(Node* x);
    int getBalance(Node* N) const;
    int height(Node* N) const;
    Node* minValueNode(Node* node);

    Node* insertHelper(Node* node, int key, bool& err);
    Node* deleteHelper(Node* node, int key, bool& err);
    bool searchHelper(Node* node, int key) const;
    void preOrderHelper(Node* node) const;
    void freeTree(Node* node);
};

