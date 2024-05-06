#include <bits/stdc++.h>
using namespace std;

// An AVL tree node
class Node {
public:
    int key;
    Node *left;
    Node *right;
    int height;

    Node* newNode(int key);
    Node(int key);
};

class AVLTree{
public:
    Node* root;
    void insert(int key);
    void deleteNode(int key);
    bool search(int key);
    void preOrder(Node* root);

private:
    Node* insertHelper(Node *node, int key);
    Node *deleteNodeHelper(Node *root, int key);
    bool searchHelper(Node* node, int key);

    int height(Node* N);
    Node* newNode(int key);
    Node* rightRotate(Node* y);
    Node* leftRotate(Node* x);
    int getBalance(Node *N);
    Node* minValueNode(Node *node);
};

