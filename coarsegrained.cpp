#include "coarsegrained.h"
#include <mutex>

Node::Node(int k) : key(k), left(nullptr), right(nullptr), height(1) {}

AVLTreeCG::AVLTreeCG() : root(nullptr), readCount(0) {}

AVLTreeCG::~AVLTreeCG() {
    freeTree(root);
}

void AVLTreeCG::freeTree(Node *node) {
	if (node == nullptr) return;
	freeTree(root->left);
	freeTree(root->right);
	delete node;
}

void AVLTreeCG::startRead() {
    readLock.lock();
    readCount++;
    if (readCount == 1) {
        writeLock.lock();
    }
    readLock.unlock();
}

void AVLTreeCG::endRead() {
    readLock.lock();
    readCount--;
    if (readCount == 0) {
        writeLock.unlock();
    }
    readLock.unlock();
}

void AVLTreeCG::startWrite() {
    writeLock.lock();
}

void AVLTreeCG::endWrite() {
    writeLock.unlock();
}

// A utility function to right rotate subtree rooted with y
Node* AVLTreeCG::rightRotate(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    return x;
}

// A utility function to left rotate subtree rooted with x
Node* AVLTreeCG::leftRotate(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    y->height = std::max(height(y->left), height(y->right)) + 1;
    return y;
}

// A utility function to get height of tree
int AVLTreeCG::height(Node* N) const {
    if (N == nullptr)
        return 0;
    return N->height;
}

// Get balance factor of node N
int AVLTreeCG::getBalance(Node* N) const {
    if (N == nullptr)
        return 0;
    return height(N->left) - height(N->right);
}

// Return the node with minimum key value in the given tree
Node* AVLTreeCG::minValueNode(Node* node) {
    Node* current = node;
    while (current->left != nullptr)
        current = current->left;
    return current;
}

// Recursive function to insert a key in the subtree rooted with node.
// Returns the new root of the subtree.
Node* AVLTreeCG::insertHelper(Node* node, int key, bool& err) {
    // 1. Perform the normal BST insertion
    if (node == nullptr)
        return new Node(key);
    if (key < node->key)
        node->left = insertHelper(node->left, key, err);
    else if (key > node->key)
        node->right = insertHelper(node->right, key, err);
    else {
        err = true;
        return node;
    }
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

// Public insert function that wraps the helper
bool AVLTreeCG::insert(int key) {
    bool err = false;
    startWrite();
    root = insertHelper(root, key, err);
    endWrite();
    return !err;
}

// Recursive function to delete a node with given key from subtree with given root.
// Returns root of the modified subtree.
Node* AVLTreeCG::deleteHelper(Node* node, int key, bool& err) {
    // STEP 1: Perform standard BST delete
    if (node == nullptr) {
        err = true;
        return node;
    }
    if (key < node->key)
        node->left = deleteHelper(node->left, key, err);
    else if (key > node->key)
        node->right = deleteHelper(node->right, key, err);
    else { // This is the node to be deleted
        if (node->left == nullptr || node->right == nullptr) {
            Node* temp = node->left ? node->left : node->right;
            if (temp == nullptr) {
                temp = node;
                node = nullptr;
            } else {
                *node = *temp;
            }
            delete temp;
        } else {
            Node* temp = minValueNode(node->right);
            node->key = temp->key;
            node->right = deleteHelper(node->right, temp->key, err);
        }
    }
    if (node == nullptr) {
        err = true;
        return node;
    }
    // Step 2: update height of the current node
    node->height = 1 + std::max(height(node->left), height(node->right));
    // Step 3: check whether this node became unbalanced
    int balance = getBalance(node);
    if (balance > 1 && getBalance(node->left) >= 0)
        return rightRotate(node);
    if (balance > 1 && getBalance(node->left) < 0) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    if (balance < -1 && getBalance(node->right) <= 0)
        return leftRotate(node);
    if (balance < -1 && getBalance(node->right) > 0) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node;
}

// Public delete function that wraps the helper
bool AVLTreeCG::deleteNode(int key) {
    bool err = false;
    startWrite();
    Node* res = deleteHelper(root, key, err);
    endWrite();
    return !err;
}

// Search for the given key in the subtree rooted with given node
bool AVLTreeCG::searchHelper(Node* node, int key) const {
    if (node == nullptr)
        return false;
    if (key == node->key)
        return true;
    if (key < node->key)
        return searchHelper(node->left, key);
    return searchHelper(node->right, key);
}

// Public search function that wraps the helper
bool AVLTreeCG::search(int key) {
    startRead();
    bool found = searchHelper(root, key);
    endRead();
    return found;
}

// A utility function to print preorder traversal of the tree.
// The function also prints the height of every node.
void AVLTreeCG::preOrderHelper(Node* node) const {
    if (node != nullptr) {
        std::cout << node->key << " ";
        preOrderHelper(node->left);
        preOrderHelper(node->right);
    }
}

// Preorder wrapper function
void AVLTreeCG::preOrder() {
    startWrite();
    std::cout << "preorder\n";
    preOrderHelper(root);
    std::cout << "\n";
    endWrite();
}
