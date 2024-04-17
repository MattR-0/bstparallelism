#include <iostream>
#include <vector>
#include <mutex>

class Node {
public:
    int key;
    Node* left;
    Node* right;
    int height;

    Node(int key);
};

class AVLTreeCG {
public:
    Node* root;
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

