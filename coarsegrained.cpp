// Coarse-grained lock implementation of AVL tree
#include <bits/stdc++.h>
#include <omp.h>
#include <string>
using namespace std;

// An AVL tree node
class Node {
    public:
    int key;
    Node *left;
    Node *right;
    int height;
};

// Operation read from file
struct Operation {
    string op;
    int key;
};

// A utility function to get height of tree
int height(Node *N) {
    if (N==NULL)
        return 0;
    return N->height;
}

// Helper function that allocates a new node with the given key
Node *newNode(int key) {
    Node *node = new Node();
    node->key = key;
    node->left = NULL;
    node->right = NULL;
    node->height = 1;
    return(node);
}

// A utility function to right rotate subtree rooted with y
Node *rightRotate(Node *y) {
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
Node *leftRotate(Node *x) {
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
int getBalance(Node *N) {
    if (N == NULL)
        return 0;
    return height(N->left)-height(N->right);
}

// Recursive function to insert a key in the subtree rooted with node and
// returns the new root of the subtree.
Node *insert(Node *node, int key) {
    // 1. Perform the normal BST insertion
    if (node==NULL)
        return(newNode(key));
    if (key < node->key)
        node->left = insert(node->left, key);
    else if (key > node->key)
        node->right = insert(node->right, key);
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
Node *minValueNode(Node *node) {
    Node *current = node;
    // Loop down to find the leftmost leaf
    while (current->left!=NULL)
        current = current->left;
    return current;
}

// Recursive function to delete a node with given key from subtree with
// given root. It returns root of the modified subtree.
Node *deleteNode(Node *root, int key) {
    // STEP 1: PERFORM STANDARD BST DELETE
    if (root==NULL)
        return root;
    // Key to be deleted lies in left subtree
    if (key < root->key)
        root->left = deleteNode(root->left, key);
    // Key to be deleted lies in right subtree
    else if (key > root->key)
        root->right = deleteNode(root->right, key);
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
            // node with two children: Get the inorder
            // successor (smallest in the right subtree)
            Node *temp = minValueNode(root->right);
            // Copy the inorder successor's
            // data to this node
            root->key = temp->key;
            // Delete the inorder successor
            root->right = deleteNode(root->right, temp->key);
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

// Search for the given key in the subtree rooted with given node
bool search(Node* node, int key) {
    if (node==NULL)
        return false;
    else if (node->key==key)
        return true;
    else if (node->key<key)
        return search(node->right, key);
    return search(node->left, key);
}

// A utility function to print preorder traversal of the tree.
// The function also prints the height of every node.
void preOrder(Node *root) {
    if(root!=NULL) {
        cout << root->key << " ";
        preOrder(root->left);
        preOrder(root->right);
    }
}

int main(int argc, char *argv[]) {
    std::string input_filename;
    int num_threads = 0;
    int opt;
    while ((opt = getopt(argc, argv, "f:n:p:i:m:b:")) != -1) {
        switch (opt) {
        case 'f':
            input_filename = optarg;
            break;
        case 'n':
            num_threads = atoi(optarg);
            break;
        default:
            std::cerr << "Usage: " << argv[0] << " -f input_filename -n num_threads [-p SA_prob] [-i SA_iters] -m parallel_mode -b batch_size\n";
            exit(EXIT_FAILURE);
        }
    }

    // Check if required options are provided
    if (empty(input_filename) || num_threads <= 0) {
        std::cerr << "Usage: " << argv[0] << " -f input_filename -n num_threads\n";
        exit(EXIT_FAILURE);
    }
    std::cout << "Input file: " << input_filename << '\n';
    std::cout << "Number of threads: " << num_threads << '\n';
    std::ifstream fin(input_filename);
    if (!fin) {
        std::cerr << "Unable to open file: " << input_filename << ".\n";
        exit(EXIT_FAILURE);
    }

    // Read input file
    int numOps;
    fin >> numOps;
    std::vector<Operation> inputOps(numOps);
    for (auto& inputOp : inputOps) {
        fin >> inputOp.op >> inputOp.key;
        if (inputOp.op!='insert' || inputOp.op!='delete' || inputOp.op!='search') {
            std::cerr << "Usage: {operator} {key}.\nInvalid operator: " << inputOp.op << ".\n";
            exit(EXIT_FAILURE);
        }
    }

    // Execute operations in parallel
    Node *root = NULL;
    omp_lock_t *coarseLock = (omp_lock_t *)calloc(1, sizeof(omp_lock_t));
    omp_init_lock(&coarseLock);
    #pragma omp parallel for
    for (int i=0; i<numOps; i++) {
        inputOp = inputOps[i];
        if (inputOp.op == 'insert') {
            omp_set_lock(&coarseLock);
            insert(root, inputOp.key);
            omp_unset_lock(&coarseLock);
        } else if (inputOp.op == 'delete') {
            omp_set_lock(&coarseLock);
            deleteNode(root, inputOp.key);
            omp_unset_lock(&coarseLock);
        } else if (inputOp.op == 'search') {
            omp_set_lock(&coarseLock);
            search(root, inputOp.key);
            omp_unset_lock(&coarseLock);
        }
    }
    preOrder(root);
}
