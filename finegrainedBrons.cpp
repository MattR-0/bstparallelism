/* Reference: https://stanford-ppl.github.io/website/papers/ppopp207-bronson.pdf */
#include <iostream>
#include <mutex>
#include <cassert>

/********************** Version manipulation constants **********************/
// Grow: Get closer to the root due to rebalancing
// Shrink: Get pushed down away from the root due to rebalancing
// Unlinked: The link between parent and child gets modified/removed
static long Unlinked = 0x1L;
static long Growing = 0x2L;
static long GrowCountIncr = 1L << 3;
static long GrowCountMask = 0xffL << 3;
static long Shrinking = 0x4L;
static long ShrinkCountIncr = 1L << 11;
static long IgnoreGrow = ~(Growing | GrowCountMask);

// Specify the rollback of optimistic concurrency control
static Object Retry = new Object();

//************************* Tree constructor **********************************/
NodeFG::NodeFG(int k) : version(0), height(1), key(k), value(0);
                        left(nullptr), right(nullptr), parent(nullptr), 
                        nodeLock() {}

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

//***************************** Utility functions *****************************/
int compare(int a, int b) {
    if (a == b) return 0;
    else if (a < b) return -1;
    else return 1;
}

/******************** Helper functions for tree operations ********************/
/* 
 * Return a child of a node based on given direction 
 */
bool AVLTreeFG::getChild(NodeFG* node, int dir) {
    assert(node != nullptr);

    if (dir == 0)
        return node->left;
    else if (dir == 1)
        return node->right;

    throw "Invalid child case \n";
    return nullptr; // error
}

/* 
 * A node can be unlinked if it has fewer than two children
 * It will be converted to a routing node otherwise
 */
bool AVLTreeFG::canUnlink(NodeFG* node) {
    return node->left == nullptr || node->right == nullptr;
}

/*
 * Right rotate subtree rooted at node
 */
void AVLTreeFG::rightRotate(NodeFG* node) {
    // Basic AVL rotation
    NodeFG* nP = node->parent;
    NodeFG* nL = node->left;
    NodeFG* nLR = nL->right;

    node->version |= Shrinking;
    nL->version |= Growing;

    node->left = nLR;
    nL->right = node;

    // Parent update
    if (nP->left == node) nP->left = nL;
    else nP->right = nL;
    nL->parent = nP;
    node->parent = nL;
    if (nLR != nullptr) nLR->parent = node;

    // Height update
    node->height = std::max(height(node->left), height(node->right)) + 1;
    nL->height = std::max(height(nL->left), height(nL->right)) + 1;

    // Version update
    nL->version += GrowCountIncr;
    node->version += ShrinkCountIncr;

    return;
}

/*
 * Left rotate subtree rooted at node
 */ 
void AVLTreeFG::leftRotate(NodeFG* node) {
    NodeFG* nP = node->parent;
    NodeFG* nR = node->right;
    NodeFG* nRL = nR->left;

    node->version |= Shrinking;
    nR->version |= Growing;

    node->right = nRL;
    nR->left = node;

    if (nP->left == node) nP->left = nR;
    else nP->right = nR;
    nR->parent = nP;
    node->parent = nR;
    if (nLR != nullptr) nRL->parent = node;

    node->height = std::max(height(node->left), height(node->right)) + 1;
    nR->height = std::max(height(nR->left), height(nR->right)) + 1;

    nR->version += GrowCountIncr;
    node->version += ShrinkCountIncr;

    return;
}

/* 
 * Get height of tree
 */
int AVLTreeFG::height(NodeFG* node) const {
    if (node == nullptr)
        return 0;
    return node->height;
}

/* 
 * Get balance factor of tree
 */
int AVLTreeFG::getBalance(NodeFG* node) const {
    if (node == nullptr)
        return 0;
    return height(node->left) - height(node->right);
}

/*
 * Calculate local height of subtree rooted at node and rebalance to maintain 
 * relaxed height invariant
 */
void AVLTreeFG::fixHeightAndRebalance(NodeFG* node) {
    // Update height of this ancestor node
    node->height = 1 + std::max(height(node->left), height(node->right));

    // Get the balance factor of this ancestor node to check this node's balance
    int balance = getBalance(node);

    // If this node becomes unbalanced, then there are 4 cases
    if (balance > 1 && key < node->left->key)
        rightRotate(node);

    else if (balance < -1 && key > node->right->key)
        leftRotate(node);

    else if (balance > 1 && key > node->left->key) {
        leftRotate(node->left);
        rightRotate(node);
    }

    else if (balance < -1 && key < node->right->key) {
        rightRotate(node->right);
        leftRotate(node);
    }
    return;
}

/* 
 * Make a certain thread block until the change bit is not set
 */
static int spinCount = 100;
void AVLTreeFG::waitUntilNotChanging(NodeFG* node) {
    long v = node->version;
    if ((v & (Growing | Shrinking)) != 0) {
        int i = 0;
        while ((node->version) == v && i < spinCount) {
            i++;
        }
        if (i == spinCount) {
            node->nodeLock.lock();
            node->nodeLock.unlock();
        } 
    }
    return;
}

/* 
* Root holder has no key, whose right child is the root. It allows all mutable 
* nodes to have a non-null parent.
*/
NodeFG* rootHolder = new NodeFG(-1);

/* Main operations: get, insert, delete */
/*
 * Public search function that wraps the helper
 */
bool AVLTreeFG::search(int key) {
    // Note: actual root is the right child of rootHolder by implementation
    return (bool) attemptSearch(key, rootHolder, 1, 0);
}

/* dir = 0: left, dir = 1: right*/
/*
 * Attempt a search of a key with hand-over-hand optimistic concurrency control
 * Return either a rollback signal, or found/not found boolean
 */
int AVLTreeFG::attemptSearch(int key, NodeFG* node, int dir, long nodeV) {
    while(true) {
        NodeFG* child = getChild(node, dir);

        // Check valid read of parent node
        // Growing the subtree with this node does not affect the correctness
        // of the current search
        if (((node.version ^ nodeV) & IgnoreGrow) != 0)
            return Retry;

        // Target is not in the tree
        if (child == nullptr)
            return NULL;

        // Target is found
        int dirNext = compare(key, child->key);
        if (dirNext == 0) return child->key;

        // At time t1: Issue a read
        // Read the associated version number v1 and block until the change bit is not set
        long childV = child.version;
        if ((childV & Shrinking) != 0) {
            waitUntilNotChanging(child);
            // Retry when blocking finishes
        }

        // Check if the link from parent to child is not modified
        // A search might become invalid if the subtree contains the key has 
        // been changed (shrink, grow, etc.) 
        else if { (childVersion != Unlinked) && (child == getChild(node, dir))
            // At time t2: Validation
            // If version stays the same, read is valid
            if (((node.version ^ nodeV) & IgnoreGrow) != 0) {
                return Retry;
            }
            // Commit
            Object p = attemptSearch(key, child, nextD, childV);

            // Read is successful
            if (p != Retry)
                return p; 
        }
    }
}

/* 
 * Public insert function that wraps the helper
 */
bool AVLTreeFG::insert(int key) {
    return attemptInsert(key, rootHolder, 1, 0);
}

/*
 * Attempt an insert of a key with optimistic concurrency control
 * Only account for insertion of new key, not update of existing key
 * Return either a rollback signal, or found/not found boolean
 */
 int AVLTreeFG::attempInsert(int key, NodeFG* node, int dir, long nodeV) {
    Object p = Retry;
    while (p == Retry) {
        NodeFG* child = getChild(node, dir);
        // Validation of parent link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0) {
            return Retry;
        }
        // Location of parent of the leaf node where new value will be inserted
        if (child == nullptr) {
            p = attemptInsertHelper(key, node, dir, nodeV);
        }
        else {
            int dirNext = compare(key, child->key);
            // Node with key exists in tree
            if (dirNext == 0) {
                // Ignore this update case
                return child->key;
            }
            else {
                long childV = child->version;
                // Child is being pushed down 
                if ((childV & Shrinking) != 0) {
                    waitUntilNotChanging(child);
                    // Retry when blocking finishes
                }
                // Child is still in the tree
                else if (childV != Unlinked && child = getChild(node, dir)) {
                    if (((node->version ^ nodeV) & IgnoreGrow) != 0) {
                        return Retry;
                    }
                    p = attempInsert(k, v, child, nextDir, chV);
                }
            }
        }
    }
    return p;
}

/*
 * To safely insert a node into the tree, we must acquire a lock on the future parent
 * of the new leaf, and we must also guarantee that no other inserting thread may 
 * decide ot perform an insertion of the same key into a different parent.\
 */
int AVLTreeFG::attempInsertHelper(int key, NodeFG* node, int dir, long nodeV) {
    // Synchronized atomic region
    node->nodeLock.lock();

    // 1. Check if the child link is null after acquiring the parent lock. 
    // 2. Any rotation that could change the parent into which k should 
    //    be inserted will invalidate the implicit range of the traversal arrived 
    //    at the parent
    if (((node->version ^ nodeV) & IgnoreGrow) != 0 || getChild(node, dir) != nullptr)
        return Retry;

    // Create new node at child pointer
    NodeFG* child = getChild(node, dir);
    child = new NodeFG(key);
    child->parent = node;

    node->nodeLock.unlock();

    fixHeightAndRebalance(node);
    return nullptr;
}

/*
 * Public delete function that wraps the helper
 */ 
bool AVLTreeFG::deleteNode(int key, NodeFG* node, int dir, long nodeV) {
    return attemptRemove(key, rootHolder, 1, 0);
}

/* 
 * Attempt delete of a key from a subtree
 * Given partially external tree design, node to be deleted will be a leaf node,
 * which allows for a fixed number of atomic operations
 */
int AVLTreeFG::attemptDeleteNode(int key, NodeFG* node, int dir, long nodeV) {
    Object p = Retry;
    while (p == Retry) {
        NodeFG* child = getChild(node, dir);
        // Validation of parent link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0) {
            return Retry;
        }
        // Key is not found
        if (child == nullptr) {
            return NULL;
        } 
        else {
            int dirNext = compare(k, child->key);
            // Node with key exists in tree
            if (dirNext == 0) {
                p = attemptRemoveNode(node, child);
            }
            else {
                long childV = child->version;
                // Child is being pushed down 
                if ((childV & Shrinking) != 0) {
                    waitUntilNotChanging(child);
                    // Retry when blocking finishes
                }
                // Child is still in the tree
                else if (childV != Unlinked && child == getChild(node, dir)) {
                    if (((node->version ^ nodeV) & IgnoreGrow) != 0) {
                        return Retry;
                    }
                    p = attemptDeleteNode(k, child, dirNext, childV);
                }
            }
        }
    }
}

/* 
 * Deletion of a node falls into two cases: 
 * (1) unlink/remove node if parent has zero or one child
 * (2) made into routing node if parent has two children 
 */
int AVLTreeFG::attemptRemoveNode(NodeFG* parent, NodeFG* node) {
    // Check that node is not a routing node
    // if (node->value == NULL) return NULL;
    
    Object prev;
    // Check if the route should be unlinked or converted into routing node 
    if (!canUnlink(node)) {
        node->nodeLock.lock();
        // Unlinking now becomes possible despite the initial state
        // Need to retry because the locks are not enough to perform unlinking
        // (acquiring lock of parent as well is needed)
        if ((node->version == Unlinked) || canUnlink(node)) {
            node->nodeLock.unlock();
            return Retry;
        }
        // Make routing node
        prev = node->value;
        node->value = NULL
        node->nodeLock.unlock()        
    }
    else {
        // Unlinking is possible here
        parent->nodeLock.lock();
        // Validation again
        if ((parent->version == Unlinked) || node->parent != parent || node->version == Unlinked) {
            parent->nodeLock.unlock();
            return Retry;
        }

        // Locks acquired for both parent and child for the unlinking to happen
        node->nodeLock.lock();
        
        // Make routing node
        prev = node->value;
        node->value = NULL;

        // Verify that unlinking is still possible --> Commit deletion
        if (canUnlink(node)) {
            // No child
            NodeFG* child = node->left ? node->left : node->right;
            if (parent->left == node) {
                parent->left = child;
            }
            else {
                parent->right = child;
            }
            // One child
            if (child != nullptr) {
                child->parent = parent;
            }
            node->version = Unlinked; // Delete node
        }
        // No need to have a rollback here
        node->nodeLock.unlock();
        parent->nodeLock.unlock();
    }
    fixHeightAndRebalance(parent);
    return prev;
}

/*
 * A utility function to print preorder traversal of the tree.
 * The function also prints the height of every node.
 */
void AVLTreeFG::preOrderHelper(NodeFG* node) const {
    if (node != nullptr) {
        std::cout << node->key << " ";
        preOrderHelper(node->left);
        preOrderHelper(node->right);
    }
}

/*
 * Preorder wrapper function
 */
void AVLTreeFG::preOrder() {
    std::cout << "preorder\n";
    preOrderHelper(root);
    std::cout << "\n";
}