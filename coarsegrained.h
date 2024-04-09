#ifndef COARSEGRAINED_H
#define COARSEGRAINED_H

#include <bits/stdc++.h>
#include <omp.h>
#include <string>

class Node {
public:
    int key;
    Node* left;
    Node* right;
    int height;

    Node(int key) : key(key), left(nullptr), right(nullptr), height(1) {}
};

class AVLTree {
public:
    Node* root;
    omp_lock_t readLock;
    omp_lock_t writeLock;

    AVLTree() : root(nullptr) {
        omp_init_lock(&readLock);
        omp_init_lock(&writeLock);
    }

    ~AVLTree() {
        omp_destroy_lock(&readLock);
        omp_destroy_lock(&writeLock);
        // Implement a recursive node deletion method to free all nodes
        // deleteNodes(root);
    }

    Node* insert(Node* node, int key);
    Node* deleteNode(Node* node, int key);
    bool search(Node* node, int key) const;
    void preOrder(Node* node) const;

    // Utility functions
    Node* rightRotate(Node* y);
    Node* leftRotate(Node* x);
    int getBalance(Node* N) const;
    int height(Node* N) const;
    Node* minValueNode(Node* node);

    // Synchronization methods
    void startWrite();
    void endWrite();
};

// Define other methods inline or in a corresponding .cpp file

#endif // COARSEGRAINED_H
