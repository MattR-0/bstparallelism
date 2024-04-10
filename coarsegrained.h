#ifndef COARSE_GRAINED_AVL_H
#define COARSE_GRAINED_AVL_H

#include <iostream>
#include <vector>
#include <omp.h>

class Node {
public:
    int key;
    Node* left;
    Node* right;
    int height;

    Node(int key);
};

class AVLTree {
public:
    Node* root;
    omp_lock_t readLock;
    omp_lock_t writeLock;

    AVLTree();
    ~AVLTree();

    Node* insert(Node* node, int key);
    Node* deleteNode(Node* node, int key);
    bool search(Node* node, int key) const;
    void preOrder(Node* node) const;

private:
    Node* rightRotate(Node* y);
    Node* leftRotate(Node* x);
    int getBalance(Node* N) const;
    int height(Node* N) const;
    Node* minValueNode(Node* node);

    void initializeLocks();
    void startRead();
    void endRead();
    void startWrite();
    void endWrite();
};

#endif
