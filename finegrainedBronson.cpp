/* Reference: https://stanford-ppl.github.io/website/papers/ppopp207-bronson.pdf */
#include <iostream>
#include <mutex>
#include <cassert>
#include "finegrainedBronson.h"

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

// Next action for each node
enum Cond : int {NothingRequired, UnlinkRequired, RebalanceRequired};

//************************* Tree constructor **********************************/
NodeFG::NodeFG(int k) : version(0), height(1), key(k), value(INT),
                        left(nullptr), right(nullptr), parent(nullptr), 
                        nodeLock() {}

/* 
* Root holder has no key, whose right child is the root. It allows all mutable 
* nodes to have a non-null parent.
*/
// std::numeric_limits<int>::min()
AVLTreeFG::AVLTreeFG() : rootHolder(new NodeFG(-1)) {}

AVLTreeFG::~AVLTreeFG() {
    freeTree(rootHolder);
}

void AVLTreeFG::freeTree(volatile NodeFG* node) {
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
 * dir = 0: left, dir = 1: right
 */
NodeFG* AVLTreeFG::getChild(NodeFG* node, int dir) {
    assert(node != nullptr);

    if (dir == 0)
        return node->left;
    else if (dir == 1)
        return node->right;

    throw "Invalid child case \n";
    return nullptr; // error
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
 * A node can be unlinked if it has fewer than two children
 * It will be converted to a routing node/marked remove otherwise
 */
bool AVLTreeFG::canUnlink(NodeFG* node) {
    return node->left == nullptr || node->right == nullptr;
}

/* 
 * Return the node with minimum key value in the given tree
 */
NodeFG* AVLTreeFG::minValueNode(NodeFG* node) {
    NodeFG* current = node;
    while (current->left != nullptr) {
        current->left->nodeLock.lock();
        current->nodeLock.unlock();
        current = current->left;
    }
    // current node (the successor node to be deleted) acquires a lock at this point
    return current;
}

// The following code regarding rebalancing and height fixing was translated 
// from the Java version of the original algorithm. As the implementation covers
// some edge cases with rebalancing, we decided to follow the original logic in
// order to perserve correctness
// Reference: https://github.com/nbronson/snaptree

/* 
 * Return label for a node during fixing interval
 */
int AVLTreeFG::nodeCondition(NodeFG* node) {
    NodeFG* left = node->left;
    NodeFG* right = node->right;

    // Unlinking a routing node (a node that marked removed but only get unlinked
    // when they have zero or one child)
    if (canUnlink(node) && (node->value == NodeFG::REM)) {
        return UnlinkRequired;
    }

    int height = node->height;
    int heightRepaired = 1 + std::max(AVLTreeFG::height(left), AVLTreeFG::height(right));
    int balance = getBalance(node);

    // In a strict AVL, balance should never exceed 1 or -1
    // However, in this given setting, balance at a local point can be affected
    // by multiple mutations and gets delayed in fixing
    if ((1 < balance) || (balance < -1)) {
        return RebalanceRequired;
    }

    return height == heightRepaired ? NothingRequired : heightRepaired; 
}

/* 
 * Assign repaired height to a node that requires it
 */
NodeFG* AVLTreeFG::fixHeightNoLock(NodeFG* node) {
    int condition = nodeCondition(node);
    switch(condition){
        case RebalanceRequired:
        case UnlinkRequired:
            return node;
        case NothingRequired:
            return nullptr;
        default:
            node->height = condition;
            return node->parent;
    }
}

/*
 * Right rotate subtree rooted at node
 */
NodeFG* AVLTreeFG::rotateRight(NodeFG* parent, NodeFG* node, NodeFG* nL, int hR, int hLL, NodeFG* nLR, int hLR) {
    // Basic AVL rotation
    NodeFG* nPL = parent->left;

    // Reader cannot read from this point due to status change
    assert(nL != nullptr);
    node->version |= Shrinking;
    nL->version |= Growing;

    node->left = nLR;
    nL->right = node;

    // Parent update
    if (nPL == node) parent->left = nL;
    else parent->right = nL;
    nL->parent = parent;
    node->parent = nL;
    if (nLR != nullptr) nLR->parent = node;

    // Height update
    int heightRepaired = std::max(hLR, hR) + 1;
    node->height = heightRepaired;
    nL->height = std::max(hLL, heightRepaired) + 1;

    // Version update
    nL->version += GrowCountIncr;
    node->version += ShrinkCountIncr;

    // Rebalance nodes at higher up levels
    int balanceNodeCheck = hLR - hR;
    if (balanceNodeCheck < -1 || 1 < balanceNodeCheck) {
        // another rotation
        return node;
    }

    // might need another rotation at new parent (node left)
    // now that node has become the right child of left, we need to check the
    // balance between the left and right child of left node
    int balanceParentCheck = hLL - heightRepaired;
    if (balanceParentCheck < -1 || 1 < balanceParentCheck) {
        return nL;
    }

    return fixHeightNoLock(parent);
}

/*
 * Left rotate subtree rooted at node
 */ 
NodeFG* AVLTreeFG::rotateLeft(NodeFG* parent, NodeFG* node, NodeFG* nR, int hL, int hRR, NodeFG* nRL, int hRL) {
    assert(nR != nullptr);
    node->version |= Shrinking;
    nR->version |= Growing;

    NodeFG* nPL = parent->left;
    node->right = nRL;
    nR->left = node;

    if (nPL == node) parent->left = nR;
    else parent->right = nR;
    nR->parent = parent;
    node->parent = nR;
    if (nRL != nullptr) nRL->parent = node;

    int heightRepaired = std::max(hL, hRL) + 1;
    node->height = heightRepaired;
    nR->height = std::max(hRR, heightRepaired) + 1;

    nR->version += GrowCountIncr;
    node->version += ShrinkCountIncr;

    int balanceNodeCheck = hRL - hL;
    if (balanceNodeCheck < -1 || 1 < balanceNodeCheck) {
        return node;
    }

    int balanceParentCheck = hRR - heightRepaired;
    if (balanceParentCheck < -1 || 1 < balanceParentCheck) {
        return nR;
    }

    return fixHeightNoLock(parent);
}

/*
 * Right-Left rotate subtree rooted at node
 */
NodeFG* AVLTreeFG::rotateRightOverLeft(NodeFG* parent, NodeFG* node, NodeFG* nL, int hR, int hLL, NodeFG* nLR, int hLRL) {
    node->version |= Shrinking;
    nL->version |= Growing;

    NodeFG* nPL = parent->left;
    NodeFG* nLRL = nLR->left;
    NodeFG* nLRR = nLR->right;
    int hLRR = height(nLRR);

    node->left = nLRR;
    if (nLRR != nullptr) {
        nLRR->parent = node;
    }

    nL->right = nLRL;
    if (nLRL != nullptr) {
        nLRL->parent = nL;
    }

    nLR->left = nL;
    nL->parent = nLR;
    nLR->right = node;
    node->parent = nLR;

    if (nPL == node) {
        parent->left = nLR;
    }
    else {
        parent->right = nLR;
    }

    nLR->parent = parent;

    int heightRepaired = std::max(hLRR, hR) + 1;
    node->height = heightRepaired;
    int heightLeftRepaired = std::max(hLL, hLRL) + 1;
    nL->height = heightLeftRepaired;
    nLR->height = 1 + std::max(heightRepaired, heightLeftRepaired);

    nL->version += GrowCountIncr;
    node->version += ShrinkCountIncr;

    int balN = hLRR - hR;
    if(balN < -1 || balN > 1){
        return node;
    }

    int balLR = heightLeftRepaired - heightRepaired;
    if(balLR < -1 || balLR > 1){
        return nLR;
    }

    return fixHeightNoLock(parent);
}

/*
 * Left-Right rotate subtree rooted at node
 */
NodeFG* AVLTreeFG::rotateLeftOverRight(NodeFG* parent, NodeFG* node, NodeFG* nR, int hL, int hRR, NodeFG* nRL, int hRLR) {
    assert(nR != nullptr);
    node->version |= Shrinking;
    nR->version |= Growing;

    NodeFG* nPL = parent->left;
    NodeFG* nRLL = nRL->left;
    NodeFG* nRLR = nRL->right;
    int hRLL = height(nRLL);

    node->right = nRLL;
    if (nRLL != nullptr) {
        nRLL->parent = node;
    }

    nR->left = nRLR;
    if (nRLR != nullptr) {
        nRLR->parent = nR;
    }

    nRL->right = nR;
    nR->parent = nRL;
    nRL->left = node;
    node->parent = nRL;

    if (nPL == node) {
        parent->left = nRL;
    }
    else {
        parent->right = nRL;
    }
    nRL->parent = parent;

    int heightRepaired = std::max(hL, hRLL) + 1;
    node->height = heightRepaired;
    int heightRightRepaired = std::max(hRLR, hRR) + 1;
    nR->height = heightRightRepaired;
    nRL->height = std::max(heightRightRepaired, heightRepaired) + 1;

    nR->version += GrowCountIncr;
    node->version += ShrinkCountIncr;

    int balN = hRLL - hL;
    if (balN < -1 || balN > 1) {
        return node;
    }

    int balRL = heightRightRepaired - heightRepaired;
    if (balRL < -1 || balRL > 1) {
        return nRL;
    }
    return fixHeightNoLock(parent);
}

/*
 * Decide rotation cases
 */
NodeFG* AVLTreeFG::rebalanceToRight(NodeFG* parent, NodeFG* node, NodeFG* nL, int hR0) {
    nL->nodeLock.lock();
    int hL = nL->height;
    if (hL - hR0 <= 1) {
        return node;
    }
    else {
        NodeFG* nLR = nL->right;
        int hLL0 = height(nL->left);
        int hLR0 = height(nLR);
        if (hLL0 >= hLR0) {
            return rotateRight(parent, node, nL, hR0, hLL0, nLR, hLR0);
        }
        else {
            nLR->nodeLock.lock();
            int hLR = nLR->height;
            if (hLL0 >= hLR) {
                return rotateRight(parent, node, nL, hR0, hLL0, nLR, hLR);
            }
            else {
                int hLRL = height(nLR->left);
                int b = hLL0 - hLRL;
                if (-1 <= b && b <= 1 && !((hLL0 == 0 || hLRL == 0) && nL->version == 0)) {
                    return rotateRightOverLeft(parent, node, nL, hR0, hLL0, nLR, hLRL);
                } 
            }
            nLR->nodeLock.unlock();
        }
        return rebalanceToLeft(node, nL, nLR, hLL0);
    }
    nL->nodeLock.unlock();
}

/*
 * Decide rotation cases
 */
NodeFG* AVLTreeFG::rebalanceToLeft(NodeFG* parent, NodeFG* node, NodeFG* nR, int hL0) {
    nR->nodeLock.lock();
    int hR = nR->height;
    if (hL0 - hR >= -1) {
        return node;
    }
    else {
        NodeFG* nRL = nR->left;
        int hRL0 = height(nRL);
        int hRR0 = height(nR->right);
        if (hRR0 >= hRL0) {
            return rotateLeft(parent, node, nR, hL0, hRR0, nRL, hRL0);
        }
        else {
            nRL->nodeLock.lock();
            int hRL = height(nRL);
            if (hRR0 >= hRL) {
                return rotateLeft(parent, node, nR, hL0, hRR0, nRL, hRL);
            }
            else {
                int hRLR = height(nRL->right);
                int b = hRR0 - hRLR;
                if (-1 <= b && 1 <= b && !((hRR0 == 0 || hRLR == 0) && nR->version == 0)) {
                    return rotateLeftOverRight(parent, node, nR, hL0, hRR0, nRL, hRLR);
                }
            }
            nRL->nodeLock.unlock();
            return rebalanceToRight(node, nR, nRL, hRR0);
        }
    }
    nR->nodeLock.lock();
}

/* 
 * Fix structural imbalance issues to maintain strict AVL height invariant
 */
NodeFG* AVLTreeFG::rebalanceNoLock(NodeFG* parent, NodeFG* node) {
    NodeFG* nL = node->left;
    NodeFG* nR = node->right;

    // Unlink (delete structurally) a routing node (deleted logically, with "removed" label)
    if(canUnlink(node) && (node->value == NodeFG::REM)){
        assert(node->parent == parent);
        if(attemptUnlinkNoLock(parent, node)){
            return fixHeightNoLock(parent);
        } 
        else {
            return node;
        }
    }
    
    int height = node->height;
    int hL0 = AVLTreeFG::height(nL);
    int hR0 = AVLTreeFG::height(nR);
    int heightRepaired = 1 + std::max(hL0, hR0);
    int balance = getBalance(node);

    if(1 < balance){
        return rebalanceToRight(parent, node, nL, hR0);
    } 
    else if(balance < -1){
        return rebalanceToLeft(parent, node, nR, hL0);
    } 
    else if(height != heightRepaired) {
        // move up to the parent
        node->height = heightRepaired;
        return fixHeightNoLock(parent);
    } 
    else {
        return nullptr;
    }
}

/*
 * Calculate local height of subtree rooted at node and rebalance to maintain 
 * relaxed height invariant
 * The fix begins at error node, propagates up to the root, and stops when no
 * action required
 */
void AVLTreeFG::fixHeightAndRebalance(NodeFG* node) {
    // Performance would be impacted if these reads of height require locks
    // If one thread fails to repair correctly, there must be a case when
    // the fild is accesse by only one thread thus being atomic
    // Therfore, we don't need to use locks or optimistic retry here
    while (node != nullptr && node->parent != nullptr) {
        int condition = nodeCondition(node);
        // No further action needed
        if (condition == NothingRequired || (node->version == Unlinked)) {
            return;
        }
        // Fix height
        if ((condition != UnlinkRequired) && (condition != RebalanceRequired)) {
            node->nodeLock.lock();
            // Propagate up to parent once finished
            node = fixHeightNoLock(node);
            node->nodeLock.unlock();
        }
        else {
            // Rotation needed
            NodeFG* parent = node->parent;
            parent->nodeLock.lock();
            if ((parent->version != Unlinked) && (node->parent == parent)) {
                node->nodeLock.lock();
                // Propagate up to parent once finished
                node = rebalanceNoLock(parent, node);
                node->nodeLock.unlock();
            }
            parent->nodeLock.unlock();
            // Retry here
        }
    }
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
            if (v != node->version) {
                return;
            }
        }
        if (i == spinCount) {
            node->nodeLock.lock();
            node->nodeLock.unlock();
        } 
    }
    return;
}

/* Main operations: get, insert, delete */
/*
 * Public search function that wraps the helper
 * Start searching from the root of the tree and traverse down until either the
 * key is found or we reach null
 */
bool AVLTreeFG::search(int key) {
    while (true) {
        // Note: actual root is the right child of rootHolder by definition
        NodeFG* root = getChild(rootHolder, 1);
        // Not found
        if (root == nullptr) {
            return false;
        }
        else {
            int dirNext = compare(key, root->key);
            // Found 
            if (dirNext == 0) {
                return true;
            }
            long rootV = root->version;
            if ((rootV & Shrinking) != 0 || (rootV != Unlinked)) {
                waitUntilNotChanging(root);
            }
            // Check linking is still valid
            else if (root == rootHolder->right) {
                AVLTreeFG::Status s = attemptSearch(key, root, dirNext, rootV);
                if (s == AVLTreeFG::SUCCESS) {
                    return true;
                }
                else if (s == AVLTreeFG::FAILURE) {
                    return false;
                }
                // Retry here otherwise
            } 
        }
    }
    throw "Invalid behavior in search! \n";
    return false;
}

/*
 * Attempt a search of a key with hand-over-hand optimistic concurrency control
 * Return either a rollback signal, or found/not found boolean
 */
AVLTreeFG::Status AVLTreeFG::attemptSearch(int key, NodeFG* node, int dir, long nodeV) {
    while(true) {
        NodeFG* child = getChild(node, dir);

        // Check valid read of parent node
        // Growing the subtree with this node does not affect the correctness
        // of the current search
        if (((node->version ^ nodeV) & IgnoreGrow) != 0)
            return AVLTreeFG::RETRY;

        // Target is not in the tree
        if (child == nullptr)
            return AVLTreeFG::FAILURE;

        // Target is found
        int dirNext = compare(key, child->key);
        if (dirNext == 0) {
            if (child->value == NodeFG::REM) {
                // printf("Found a removed node \n");
                return AVLTreeFG::FAILURE;
            }
            return AVLTreeFG::SUCCESS;
        }

        // At time t1: Issue a read
        // Read the associated version number v1 and block until the change bit is not set
        long childV = child->version;
        if ((childV & Shrinking) != 0) {
            waitUntilNotChanging(child);
            // Retry when blocking finishes
        }

        // Check if the link from parent to child is not modified
        // A search might become invalid if the subtree contains the key has 
        // been changed (shrink, grow, etc.) 
        else if (childV != Unlinked && (child == getChild(node, dir))) {
            // At time t2: Validation
            // If version stays the same, read is valid
            if (((node->version ^ nodeV) & IgnoreGrow) != 0) {
                return AVLTreeFG::RETRY;
            }
            // Commit
            AVLTreeFG::Status p = attemptSearch(key, child, dirNext, childV);

            // Read is successful
            if (p != AVLTreeFG::RETRY)
                return p; 
        }
    }
}

/* 
 * Public insert function that wraps the helper
 */
bool AVLTreeFG::insert(int key) {
    while (true) {
        NodeFG* root = getChild(rootHolder, 1);
        // Insert into null root
        if (root == nullptr) {
            rootHolder->nodeLock.lock();
            rootHolder->right = new NodeFG(key);
            rootHolder->height = 2;
            rootHolder->nodeLock.unlock();
            return true;
        }
        else {
            int dirNext = compare(key, root->key);
            // Ignore case of updating existing key 
            if (dirNext == 0) {
                return false;
            }
            long rootV = root->version;
            if ((rootV & Shrinking) != 0 || (rootV != Unlinked)) {
                waitUntilNotChanging(root);
            }
            // Check linking is still valid
            else if (root == rootHolder->right) {
                AVLTreeFG::Status s = attemptInsert(key, root, dirNext, rootV);
                if (s == AVLTreeFG::SUCCESS) {
                    return true;
                }
                else if (s == AVLTreeFG::FAILURE) {
                    return false;
                }
                // Retry here otherwise
            } 
        }
    }
    throw "Invalid behavior in insert! \n";
    return false;
}

/*
 * Attempt an insert of a key with optimistic concurrency control
 * Only account for insertion of new key, not update of existing key
 * Return either a rollback signal, or found/not found boolean
 */
 AVLTreeFG::Status AVLTreeFG::attemptInsert(int key, NodeFG* node, int dir, long nodeV) {
    AVLTreeFG::Status p = AVLTreeFG::RETRY;
    while (p == AVLTreeFG::RETRY) {
        NodeFG* child = getChild(node, dir);
        // Validation of parent link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0) {
            return AVLTreeFG::RETRY;
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
                return AVLTreeFG::FAILURE;
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
                        return AVLTreeFG::RETRY;
                    }
                    p = attemptInsert(key, child, dirNext, childV);
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
AVLTreeFG::Status AVLTreeFG::attemptInsertHelper(int key, NodeFG* node, int dir, long nodeV) {
    // Synchronized atomic region
    node->nodeLock.lock();

    // 1. Check if the child link is null after acquiring the parent lock. 
    // 2. Any rotation that could change the parent into which k should 
    //    be inserted will invalidate the implicit range of the traversal arrived 
    //    at the parent
    if (((node->version ^ nodeV) & IgnoreGrow) != 0 || getChild(node, dir) != nullptr)
        return AVLTreeFG::RETRY;

    // Create new node at child pointer
    NodeFG* child = getChild(node, dir);
    child = new NodeFG(key);
    child->parent = node;

    node->nodeLock.unlock();

    fixHeightAndRebalance(node);
    return AVLTreeFG::SUCCESS;
}

/*
 * Public delete function that wraps the helper
 */ 
bool AVLTreeFG::deleteNode(int key) {
 while (true) {
        NodeFG* root = getChild(rootHolder, 1);
        // Insert into null root
        if (root == nullptr) {
            printf("Delete from empty tree! \n");
            return false;
        }
        else {
            int dirNext = compare(key, root->key);
            // Found node to be deleted
            if (dirNext == 0) {
                if(attemptRemoveNode(rootHolder, root) == AVLTreeFG::SUCCESS) {
                    return true;
                }
                else {
                    return false;
                }
            }
            long rootV = root->version;
            if ((rootV & Shrinking) != 0 || (rootV != Unlinked)) {
                waitUntilNotChanging(root);
            }
            // Check linking is still valid
            else if (root == rootHolder->right) {
                AVLTreeFG::Status s = attemptDeleteNode(key, root, dirNext, rootV);
                if (s == AVLTreeFG::SUCCESS) {
                    return true;
                }
                else if (s == AVLTreeFG::FAILURE) {
                    return false;
                }
                // Retry here otherwise
            } 
        }
    }
    throw "Invalid behavior in delete! \n";
    return false;
}

/* 
 * Attempt delete of a key from a subtree
 * Given partially external tree design, node to be deleted will be a leaf node,
 * which allows for a fixed number of atomic operations
 */
AVLTreeFG::Status AVLTreeFG::attemptDeleteNode(int key, NodeFG* node, int dir, long nodeV) {
    AVLTreeFG::Status p = AVLTreeFG::RETRY;
    while (p == AVLTreeFG::RETRY) {
        NodeFG* child = getChild(node, dir);
        // Validation of parent link
        if (((node->version ^ nodeV) & IgnoreGrow) != 0) {
            return AVLTreeFG::RETRY;
        }
        // Key is not found
        if (child == nullptr) {
            return AVLTreeFG::FAILURE;
        } 
        else {
            int dirNext = compare(key, child->key);
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
                        return AVLTreeFG::RETRY;
                    }
                    p = attemptDeleteNode(key, child, dirNext, childV);
                }
            }
        }
    }
    return AVLTreeFG::FAILURE;
}

/*
 * Attempt to unlink a node (delete structurally) from its parent
 */
bool AVLTreeFG::attemptUnlinkNoLock(NodeFG* parent, NodeFG* node){
    if((parent->left != node && parent->right != node) || (node->parent != parent)){
        return false;
    }
    NodeFG* child = node->left ? node->left : node->right;
    // Zero child
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
    node->value = NodeFG::REM;
    return true;
}

/* 
 * Deletion of a node falls into two cases: 
 * (1) unlink/remove node if parent has zero or one child
 * (2) made into routing node if parent has two children 
 */
AVLTreeFG::Status AVLTreeFG::attemptRemoveNode(NodeFG* parent, NodeFG* node) {
    // Check that node is not a routing/removed node
    if (node->value == NodeFG::REM) {
        printf("Trying to delete removed node \n");
        return AVLTreeFG::FAILURE;
    }
    
    NodeFG::NodeType prev;
    // Check if the route should be unlinked or converted into routing node 
    if (!canUnlink(node)) {
        node->nodeLock.lock();
        // Unlinking now becomes possible despite the initial state
        // Need to retry because the locks are not enough to perform unlinking
        // (acquiring lock of parent as well is needed)
        if ((node->version == Unlinked) || canUnlink(node)) {
            node->nodeLock.unlock();
            return AVLTreeFG::RETRY;
        }
        // Make routing/marked removed node
        prev = node->value;
        assert(prev == NodeFG::INT);
        node->value = NodeFG::REM;
        node->nodeLock.unlock();    
    }
    else {
        // Unlinking is possible here
        parent->nodeLock.lock();
        // Validation again
        if ((parent->version == Unlinked) || node->parent != parent || node->version == Unlinked) {
            parent->nodeLock.unlock();
            return AVLTreeFG::RETRY;
        }

        // Locks acquired for both parent and child for the unlinking to happen
        node->nodeLock.lock();
        
        // Make routing/mark removed node
        prev = node->value;
        assert(prev == NodeFG::INT);
        node->value = NodeFG::REM;

        // Verify that unlinking is still possible --> Commit deletion
        if (canUnlink(node)) {
            attemptUnlinkNoLock(parent, node);
        }
        // No need to have a rollback here
        node->nodeLock.unlock();
        parent->nodeLock.unlock();
    }
    fixHeightAndRebalance(parent);
    return AVLTreeFG::SUCCESS;
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
    preOrderHelper(rootHolder);
    std::cout << "\n";
}