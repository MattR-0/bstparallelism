// Sequential implementation of AVL tree
#include <bits/stdc++.h>
#include "sequential.h"
using namespace std;

// A utility function to get height of tree
int AVLTree::height(Node *N) {
    if (N==NULL)
        return 0;
    return N->height;
}

Node::Node(int k) : key(k), left(nullptr), right(nullptr), height(1) {}

// A utility function to right rotate subtree rooted with y
Node* AVLTree::rightRotate(Node *y) {
    Node *x = y->left;
    Node *T2 = x->right;
    // Perform rotation
    x->right = y;
    y->left = T2;
    // Update heights
    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    // Return new root
    return x;
}

// A utility function to left rotate subtree rooted with x
Node* AVLTree::leftRotate(Node *x) {
    Node *y = x->right;
    Node *T2 = y->left;
    // Perform rotation
    y->left = x;
    x->right = T2;
    // Update heights
    x->height = std::max(height(x->left), height(x->right))+1;
    y->height = std::max(height(y->left), height(y->right))+1;
    // Return new root
    return y;
}

// Get balance factor of node N
int AVLTree::getBalance(Node *N) {
    if (N == NULL)
        return 0;
    return height(N->left)-height(N->right);
}

// Public insert function that wraps the helper
void AVLTree::insert(int key) {
    root = insertHelper(root, key);
}

// Recursive function to insert a key in the subtree rooted with node and
// returns the new root of the subtree.
Node* AVLTree::insertHelper(Node *node, int key) {
    // 1. Perform the normal BST insertion
    if (node==NULL)
        return(new Node(key));
    if (key < node->key)
        node->left = insertHelper(node->left, key);
    else if (key > node->key)
        node->right = insertHelper(node->right, key);
    else // Equal keys are not allowed in BST
        return node;
    // 2. Update height of this ancestor node
    node->height = 1 + std::max(height(node->left), height(node->right));
    // 3. Get the balance factor of this ancestor node to check this node's balance
    int balance = getBalance(node);

    // If this node becomes unbalanced, then there are 4 cases
    // Left Left Case
    if (balance>1 && key<node->left->key)
        return rightRotate(node);
    // Right Right Case
    if (balance<-1 && key>node->right->key)
        return leftRotate(node);
    // Left Right Case
    if (balance>1 && key>node->left->key) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }
    // Right Left Case
    if (balance < -1 && key < node->right->key) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    // Return the (unchanged) node pointer
    return node;
}

// Return the node with minimum key value in the given tree.
Node* AVLTree::minValueNode(Node *node) {
    Node *current = node;
    // Loop down to find the leftmost leaf
    while (current->left!=NULL)
        current = current->left;
    return current;
}

// Public delete function that wraps the helper
void AVLTree::deleteNode(int key) {
    root = deleteNodeHelper(root, key);
}

// Recursive function to delete a node with given key from subtree with
// given root. It returns root of the modified subtree.
Node* AVLTree::deleteNodeHelper(Node *root, int key) {
    // STEP 1: PERFORM STANDARD BST DELETE
    if (root==NULL)
        return root;
    // Key to be deleted lies in left subtree
    if (key < root->key)
        root->left = deleteNodeHelper(root->left, key);
    // Key to be deleted lies in right subtree
    else if (key > root->key)
        root->right = deleteNodeHelper(root->right, key);
    // This is the node to be deleted
    else {
        if (root->left==NULL || root->right==NULL) {
            Node *temp = root->left ? root->left : root->right;
            // No child case
            if (temp == NULL) {
                temp = root;
                root = NULL;
            }
            else
                *root = *temp; // Copy the contents of the non-empty child
            free(temp);
        }
        else {
            // Get the inorder successor (smallest in the right subtree)
            Node *temp = minValueNode(root->right);
            // Copy the inorder successor's data to this node
            root->key = temp->key;
            // Delete the inorder successor
            root->right = deleteNodeHelper(root->right, temp->key);
        }
    }
    // If the tree had only one node then return
    if (root==NULL)
        return root;

    // Step 2: update height of the current node 
    root->height = 1+std::max(height(root->left), height(root->right));
    // Step 3: check whether this node became unbalanced
    int balance = getBalance(root);
    // Left Left Case
    if (balance>1 && getBalance(root->left)>=0)
        return rightRotate(root);
    // Left Right Case
    if (balance>1 && getBalance(root->left)<0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }
    // Right Right Case
    if (balance<-1 && getBalance(root->right)<=0)
        return leftRotate(root);
    // Right Left Case
    if (balance<-1 && getBalance(root->right)>0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }
    return root;
}

// Public search function that wraps the helper
bool AVLTree::search(int key) {
    bool found = searchHelper(root, key);
    return found;
}

// Search for the given key in the subtree rooted with given node
bool AVLTree::searchHelper(Node* node, int key) {
    if (node==NULL)
        return false;
    else if (node->key==key)
        return true;
    else if (node->key<key)
        return searchHelper(node->right, key);
    return searchHelper(node->left, key);
}

// A utility function to print preorder traversal of the tree.
// The function also prints the height of every node.
void AVLTree::preOrder(Node *root) {
    if(root!=NULL) {
        cout << root->key << " ";
        preOrder(root->left);
        preOrder(root->right);
    }
}

// // Driver Code
// int main() {
//     AVLTree* avl_tree = new AVLTree(); // Create an instance of AVLTree
//     // Constructing example tree
//     avl_tree->insert(10);
//     avl_tree->insert(20);
//     avl_tree->insert(30);
//     avl_tree->insert(40);
//     avl_tree->insert(50);
//     avl_tree->insert(25);

//     /* The constructed AVL Tree would be
//             30
//             / \
//            20 40
//           / \   \
//          10 25  50
//     */
//     std::cout << "Preorder traversal of the constructed AVL tree: \n";
//     avl_tree->preOrder(avl_tree->root);
//     printf("\n");

//     avl_tree->deleteNode(10);

//     /* The AVL Tree after deletion of 10
//          1
//         / \
//        0   9
//       /   / \
//     -1   5   11
//         / \
//        2   6
//     */
//     std::cout << "Preorder traversal after deletion of 10: \n";
//     avl_tree->preOrder(avl_tree->root);
//     printf("\n");
    
//     return 0;
// }