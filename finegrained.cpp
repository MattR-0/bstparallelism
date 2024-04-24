#include "finegrained.h"
#include <iostream>
#include <mutex>

NodeFG::NodeFG(int k) : key(k), left(nullptr), right(nullptr), height(1), nodeLock() {}

AVLTreeFG::AVLTreeFG() : root(nullptr) {}

AVLTreeFG::~AVLTreeFG() {
    freeTree(root);
}

void AVLTreeFG::freeTree(NodeFG* node) {
	if (node == nullptr) return;
	freeTree(node->left);
	freeTree(node->right);
	delete node;
}

// A utility function to right rotate subtree rooted with y
NodeFG* AVLTreeFG::rightRotate(NodeFG* y) {
    NodeFG* x = y->left;
    NodeFG* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    return x;
}

// A utility function to left rotate subtree rooted with x
NodeFG* AVLTreeFG::leftRotate(NodeFG* x) {
    NodeFG* y = x->right;
    NodeFG* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    y->height = std::max(height(y->left), height(y->right)) + 1;
    return y;
}

// A utility function to get height of tree
int AVLTreeFG::height(NodeFG* N) const {
    if (N == nullptr)
        return 0;
    return N->height;
}

// Get balance factor of node N
int AVLTreeFG::getBalance(NodeFG* N) const {
    if (N == nullptr)
        return 0;
    return height(N->left) - height(N->right);
}

// Return the node with minimum key value in the given tree
NodeFG* AVLTreeFG::minValueNode(NodeFG* node) {
    NodeFG* current = node;
    while (current->left != nullptr) {
        current->left->nodeLock.lock();
        current->nodeLock.unlock();
        current = current->left;
    }
    // current node (the leaf node to be deleted) acquires a lock at this point
    return current;
}

void AVLTreeFG::insertHelper(NodeFG* node, int key, bool& err) {
    if (node == nullptr)
        throw "Invalid insertion of node\n";
    if (key < node->key) {
        if (node->left == NULL) {
            // Critical section: the parent is protected, so only one thread can update the child at a time
            node->left = new NodeFG(key); 
            // Release control once child is inserted
            node->nodeLock.unlock(); 
            // Check for balance after insertion
        }
        else {
            // Hand-over-hand pattern:
            // The child acquires the lock first, then the parent releases the lock
            node->left->nodeLock.lock();
            node->nodeLock.unlock();
            insertHelper(node->left, key, err);
        }
    }
    else if (key > node->key) {
        if (node->right == NULL) {
            node->right = new NodeFG(key);
            node->nodeLock.unlock();
        }
        else {
            node->right->nodeLock.lock();
            node->nodeLock.unlock();
            insertHelper(node->right, key, err);
        }
    }
    else {
        err = true;
        return;
    }
    // 2. Update height of this ancestor node
    node->height = 1 + std::max(height(node->left), height(node->right));
    // 3. Get the balance factor of this ancestor node to check this node's balance
    int balance = getBalance(node);
    // If this node becomes unbalanced, then there are 4 cases
    if (balance > 1 && key < node->left->key)
        node = rightRotate(node);
    else if (balance < -1 && key > node->right->key)
        node =  leftRotate(node);
    else if (balance > 1 && key > node->left->key) {
        node->left = leftRotate(node->left);
        node = rightRotate(node);
    }
    else if (balance < -1 && key < node->right->key) {
        node->right = rightRotate(node->right);
        node = leftRotate(node);
    }
    return;
}

// Public insert function that wraps the helper
bool AVLTreeFG::insert(int key) {
    bool err = false;
    rootLock.lock();
    if (root == nullptr) {
        root = new NodeFG(key);
        return !err;
    }
    rootLock.unlock();
    root->nodeLock.lock();
    insertHelper(root, key, err);
    return !err;
}

// Recursive function to delete a node with given key from subtree with given root.
void AVLTreeFG::deleteHelper(NodeFG* node, int key, bool& err) {
    // STEP 1: Perform standard BST delete
    if (node == nullptr) {
        err = true;
        root->nodeLock.unlock();
        return;
    }
    if (key < node->key) {
        // Hand-over-hand locking until the node to be deleted is found
        node->left->nodeLock.lock();
        node->nodeLock.unlock();
        deleteHelper(node->left, key, err);
    }
    else if (key > node->key) {
        node->right->nodeLock.lock();
        node->nodeLock.unlock();
        deleteHelper(node->right, key, err);
    }
    else { // This is the node to be deleted
        if (node->left == nullptr || node->right == nullptr) {
            NodeFG* temp = node->left ? node->left : node->right;
            if (temp == nullptr) { // Leaf node
                delete node;
            } else {
                // Height will be updated below while rebalancing
                temp->nodeLock.lock();
                node->key = temp->key;  
                node->left = temp->left; 
                node->right = temp->right;  
                temp->nodeLock.unlock();
                delete temp;
                node->nodeLock.unlock();
            }
        } else {
            node->right->nodeLock.lock();
            NodeFG* temp = minValueNode(node->right);
            node->key = temp->key; // both nodes should have lock here
            temp->nodeLock.unlock();
            delete temp;
            node->nodeLock.unlock(); // deletion complete

            // Recursive call is not necessary here (?) 
            // Temp is a leaf so "delete" might be enough. 
            // If insertion commits correctly (i.e., during deletion, insert to 
            // temp position happens then temp must witness new inserted value 
            // not the old one)
            // deleteHelper(node->right, temp->key, err);
        }
    }
    if (node == nullptr) {
        err = true;
        return;
    }
    // Step 2: update height of the current node
    node->height = 1 + std::max(height(node->left), height(node->right));
    // Step 3: check whether this node became unbalanced
    int balance = getBalance(node);
    if (balance > 1 && getBalance(node->left) >= 0)
        node = rightRotate(node);
    if (balance > 1 && getBalance(node->left) < 0) {
        node->left = leftRotate(node->left);
        node = rightRotate(node);
    }
    if (balance < -1 && getBalance(node->right) <= 0)
        node = leftRotate(node);
    if (balance < -1 && getBalance(node->right) > 0) {
        node->right = rightRotate(node->right);
        node = leftRotate(node);
    }
    return;
}

// Public delete function that wraps the helper
bool AVLTreeFG::deleteNode(int key) {
    bool err = false;
    root->nodeLock.lock();
    deleteHelper(root, key, err);
    return !err;
}

// Search for the given key in the subtree rooted with given node
bool AVLTreeFG::searchHelper(NodeFG* node, int key) const {
    if (node == nullptr)
        return false;
    if (key == node->key)
        return true;
    if (key < node->key)
        return searchHelper(node->left, key);
    return searchHelper(node->right, key);
}

// Public search function that wraps the helper
bool AVLTreeFG::search(int key) {
    bool found = searchHelper(root, key);
    return found;
}

// A utility function to print preorder traversal of the tree.
// The function also prints the height of every node.
void AVLTreeFG::preOrderHelper(NodeFG* node) const {
    if (node != nullptr) {
        std::cout << node->key << " ";
        preOrderHelper(node->left);
        preOrderHelper(node->right);
    }
}

// Preorder wrapper function
void AVLTreeFG::preOrder() {
    std::cout << "preorder\n";
    preOrderHelper(root);
    std::cout << "\n";
}

int main() {
    return 0;
}