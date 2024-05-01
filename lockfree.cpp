#include "lockfree.h"

static long Unlinked = 0x1L;
static long Growing = 0x2L;
static long GrowCountIncr = 1L << 3;
static long GrowCountMask = 0xffL << 3;
static long Shrinking = 0x4L;
static long ShrinkCountIncr = 1L << 11;
static long IgnoreGrow = ~(Growing | GrowCountMask);
static Object Retry = new Object();

NodeLF* NodeLF::child(int direction) {
    if (direction==1)
        return right;
    else if (direction==-1)
        return left;
}

int get(int k) {
    return (int)attemptGet(k, rootHolder, 1, 0);
}

Object AVLTreeLF::attemptGet(int k, NodeLF node, int dir, long nodeV) {
    while (true) {
        NodeLF child = node.child(dir);
        if (((node.version^nodeV) & IgnoreGrow) != 0)
            return Retry;
        if (child == nullptr)
            return nullptr;
        int nextD = k.compareTo(child.key);
        if (nextD == 0)
            return child.value;
        long chV = child.version;
        if ((chV & Shrinking) != 0) {
            waitUntilNotChanging(child);
        } else if (chV != Unlinked && child == node.child(dir)) {
            if (((node.version^nodeV) & IgnoreGrow) != 0)
                return Retry;
            Object p = attemptGet(k, child, nextD, chV);
            if (p != Retry)
                return p;
        }
    }
}

int AVLTreeLF::put(int k, int v) {
    return (int)attemptPut(k, v, rootHolder, 1, 0);
}

void* AVLTreeLF::attemptPut(int k, int v, NodeLF* node, int dir, long nodeV) {
    void* p = reinterpret_cast<void*>(-1); // Retry represented as -1
    do {
        NodeLF* child = node->child(dir);
        if (((node->version.load() ^ nodeV) & IgnoreGrow) != 0)
            return reinterpret_cast<void*>(-1); // Retry
        if (child == nullptr) {
            p = attemptInsert(k, v, node, dir, nodeV);
        } else {
            int nextD = (k < child->key) ? -1 : (k > child->key) ? 1 : 0;
            if (nextD == 0) {
                p = attemptUpdate(child, v);
            } else {
                long chV = child->version.load();
                if ((chV & Shrinking) != 0) {
                    waitUntilNotChanging(child);
                } else if (chV != Unlinked && child == node->child(dir)) {
                    if (((node->version.load() ^ nodeV) & IgnoreGrow) != 0)
                        return reinterpret_cast<void*>(-1); // Retry
                    p = attemptPut(k, v, child, nextD, chV);
                }
            }
        }
    } while (p == reinterpret_cast<void*>(-1));
    return p;
}

std::atomic<void*> AVLTreeLF::attemptInsert(int k, int v, NodeLF* node, int dir, long nodeV) {
    NodeLF* expectedChild = nullptr;
    if ((node->version.load() ^ nodeV) & IgnoreGrow || node->child(dir) != nullptr) {
        return reinterpret_cast<void*>(-1);
    }

    NodeLF* newNode = new NodeLF(k, v, node, 0, nullptr, nullptr);
    if (node->child(dir) == nullptr) {
        std::atomic<NodeLF*>& child = (dir == 1) ? node->right : node->left;
        if (child.compare_exchange_strong(expectedChild, newNode)) {
            // Fix height and rebalance after successful insertion
            fixHeightAndRebalance(node);
            return nullptr;  // Indicate success with null
        } else {
            delete newNode; // Cleanup if CAS fails
            return reinterpret_cast<void*>(-1);
        }
    }
    return reinterpret_cast<void*>(-1);
}

std::atomic<void*> AVLTreeLF::attemptUpdate(NodeLF* node, int v) {
    if (node->version.load() == Unlinked) return reinterpret_cast<void*>(-1);
    int prevValue = node->value.exchange(v);
    return reinterpret_cast<void*>(prevValue);
}


int AVLTreeLF::remove(int k) {
    return (int)attemptRemove(k, rootHolder, 1, 0);
}

... // attemptRemove is similar to attemptPut

bool AVLTreeLF::canUnlink(Node n) {
    return n.left == null || n.right == null;
}

Object AVLTreeLF::attemptRmNode(Node par, Node n) {
    if (n.value == null) return null;
        Object prev;
    if (!canUnlink(n)) {
        synchronized (n) {
            if (n.version == Unlinked || canUnlink(n))
            return Retry;
            prev = n.value;
            n.value = null;
        }
    } else {
        synchronized (par) {
            if (par.version == Unlinked || n.parent != par || n.version == Unlinked)
                return Retry;
            synchronized (n) {
                prev = n.value;
                n.value = null;
                if (canUnlink(n)) {
                    Node c = n.left == null ? n.right : n.left;
                    if (par.left == n)
                        par.left = c;
                    else
                        par.right = c;
                    if (c != null) c.parent = par;
                    n.version = Unlinked;
                }
            }
        }
        fixHeightAndRebalance(par);
    }
    return prev;
}

// n.parent, n, and n.left are locked on entry
void AVLTreeLF::rotateRight(Node n) {
    Node nP = n.parent;
    Node nL = n.left;
    Node nLR = nL.right;
    n.version |= Shrinking;
    nL.version |= Growing;
    n.left = nLR;
    nL.right = n;
    if (nP.left == n) nP.left = nL; else nP.right = nL;
    nL.parent = nP;
    n.parent = nL;
    if (nLR != null) nLR.parent = n;
    int h = 1 + Math.max(height(nLR), height(n.right));
    n.height = h;
    nL.height = 1 + Math.max(height(nL.left), h);
    nL.version += GrowCountIncr;
    n.version += ShrinkCountIncr;
}

static int SpinCount = 100;

void AVLTreeLF::waitUntilNotChanging(Node n) {
    long v = n.version;
    if ((v & (Growing | Shrinking)) != 0) {
        int i = 0;
        while (n.version == v && i < SpinCount) ++i;
        if (i == SpinCount) synchronized (n) { };
    }
}

NodeLF::NodeLF(int k) : key(k), parent(nullptr), left(nullptr), right(nullptr), height(1), version(1) {}

AVLTreeLF::AVLTreeLF() : root(nullptr) {}

AVLTreeLF::~AVLTreeLF() {
    freeTree(root);
}

void AVLTreeLF::freeTree(NodeLF* node) {
	if (node == nullptr) return;
	freeTree(node->left);
	freeTree(node->right);
	delete node;
}
