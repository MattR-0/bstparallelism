#include "coarsegrained.h"

Node::Node(int k) : key(k), left(nullptr), right(nullptr), height(1) {}

AVLTree::AVLTree() : root(nullptr), readCount(0) {
    omp_init_lock(&writeLock);
    omp_init_lock(&readLock);
}

AVLTree::~AVLTree() {
    // Ideally, add a method to recursively delete nodes to avoid memory leaks
    omp_destroy_lock(&writeLock);
    omp_destroy_lock(&readLock);
}

void AVLTree::startRead() {
    omp_set_lock(&readLock);
    readCount++;
    if (readCount == 1) {
        omp_set_lock(&writeLock);  // First reader acquires write lock
    }
    omp_unset_lock(&readLock);
}

void AVLTree::endRead() {
    omp_set_lock(&readLock);
    readCount--;
    if (readCount == 0) {
        omp_unset_lock(&writeLock);  // Last reader releases write lock
    }
    omp_unset_lock(&readLock);
}

void AVLTree::startWrite() {
    omp_set_lock(&writeLock);
}

void AVLTree::endWrite() {
    omp_unset_lock(&writeLock);
}

// A utility function to right rotate subtree rooted with y
Node* AVLTree::rightRotate(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    return x;
}

// A utility function to left rotate subtree rooted with x
Node* AVLTree::leftRotate(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    y->height = std::max(height(y->left), height(y->right)) + 1;
    return y;
}

// A utility function to get height of tree
int AVLTree::height(Node* N) const {
    if (N == nullptr)
        return 0;
    return N->height;
}

// Get balance factor of node N
int AVLTree::getBalance(Node* N) const {
    if (N == nullptr)
        return 0;
    return height(N->left) - height(N->right);
}

// Return the node with minimum key value in the given tree
Node* AVLTree::minValueNode(Node* node) {
    Node* current = node;
    while (current->left != nullptr)
        current = current->left;
    return current;
}

// Recursive function to insert a key in the subtree rooted with node.
// Returns the new root of the subtree.
Node* AVLTree::insert(Node* node, int key) {
    // 1. Perform the normal BST insertion
    if (node == nullptr)
        return new Node(key);
    if (key < node->key)
        node->left = insert(node->left, key);
    else if (key > node->key)
        node->right = insert(node->right, key);
    else
        return node;
    // 2. Update height of this ancestor node
    node->height = 1 + std::max(height(node->left), height(node->right));
    // 3. Get the balance factor of this ancestor node to check this node's balance
    int balance = getBalance(node);
    // If this node becomes unbalanced, then there are 4 cases
    if (balance > 1 && key < node->left->key)
        return rightRotate(node);
    if (balance < -1 && key > node->right->key)
        return leftRotate(node);
    if (balance > 1 && key > node->left->key) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && key < node->right->key) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node;
}

// Recursive function to delete a node with given key from subtree with given root.
// Returns root of the modified subtree.
Node* AVLTree::deleteNode(Node* root, int key) {
    // STEP 1: Perform standard BST delete
    if (root == nullptr)
        return root;
    if (key < root->key)
        root->left = deleteNode(root->left, key);
    else if (key > root->key)
        root->right = deleteNode(root->right, key);
    else { // This is the node to be deleted
        if (root->left == nullptr || root->right == nullptr) {
            Node* temp = root->left ? root->left : root->right;
            if (temp == nullptr) {
                temp = root;
                root = nullptr;
            } else {
                *root = *temp;
            }
            delete temp;
        } else {
            Node* temp = minValueNode(root->right);
            root->key = temp->key;
            root->right = deleteNode(root->right, temp->key);
        }
    }
    if (root == nullptr)
      return root;
    // Step 2: update height of the current node
    root->height = 1 + std::max(height(root->left), height(root->right));
    // Step 3: check whether this node became unbalanced
    int balance = getBalance(root);
    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);
    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);
    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    return root;
}

// Search for the given key in the subtree rooted with given node
bool AVLTree::search(Node* node, int key) const {
    if (node == nullptr)
        return false;
    if (key == node->key)
        return true;
    if (key < node->key)
        return search(node->left, key);
    return search(node->right, key);
}

// A utility function to print preorder traversal of the tree.
// The function also prints the height of every node.
void AVLTree::preOrder(Node* node) const {
    if (node != nullptr) {
        std::cout << node->key << " ";
        preOrder(node->left);
        preOrder(node->right);
    }
}
