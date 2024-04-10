#include <iostream>
#include <algorithm>
#include <stdexcept>

int checkHeightAndBalance(Node* node) {
    if (!node) return 0;
    int leftHeight = checkHeightAndBalance(node->left);
    int rightHeight = checkHeightAndBalance(node->right);
    // Check that node height is correct
    if (node->height != 1 + std::max(leftHeight, rightHeight)) {
        throw std::runtime_error("Node height is incorrect.");
    }
    // Check balance factor
    int balance = leftHeight - rightHeight;
    if (balance < -1 || balance > 1) {
        throw std::runtime_error("Node is unbalanced.");
    }
    // Check that child keys are correct
    if (node->left && node->left->key >= node->key) {
        throw std::runtime_error("Left child key is greater or equal to node key.");
    }
    if (node->right && node->right->key <= node->key) {
        throw std::runtime_error("Right child key is lesser or equal to node key.");
    }
    // Check for circular references
    if (node->left == node || node->right == node) {
        throw std::runtime_error("Circular reference detected.");
    }
    return node->height;
}

void basicSanityCheck(node *tree) {
    try {
        checkHeightAndBalance(tree);
        std::cout << "Basic sanity check passed." << std::endl;
    } catch (std::runtime_error& e) {
        std::cerr << "Sanity check failed: " << e.what() << std::endl;
    }
}

// Usage
int main() {
    basicSanityCheck(avl);
    return 0;
}
