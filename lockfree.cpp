#include "lockfree.h"

#define UNLINKED 0x1L
#define GROWING 0x2L
#define GROWCOUNTINCR 1L << 3
#define GROWCOUNTMASK 0xffL << 3
#define SHRINKING 0x4L
#define SHRINKCOUNTINCR 1L << 11
#define IGNOREGROW ~(GROWING | GROWCOUNTMASK)
#define SPINCOUNT 100

NodeLF* NodeLF::child(int direction) {
    if (direction==1)
        return right;
    else if (direction==-1)
        return left;
}

bool AVLTreeLF::search(int k) {
    return (bool)attemptSearch(k, rootHolder, 1, 0);
}

int AVLTreeLF::attemptSearch(int k, NodeLF node, int dir, long nodeV) {
    while (true) {
        NodeLF* child = node->child(dir);
        if (((node->version.load()^nodeV)&IGNOREGROW) != 0)
            return -1;
        if (child==nullptr)
            return 0;
        int nextD = (k<child->key) ? -1 : (k>child->key) ? 1 : 0;
        if (nextD==0)
            return 1;
        long chV = child->version.load();
        if ((chV&SHRINKING) != 0) {
            waitUntilNotChanging(child);
        } else if (chV!=UNLINKED && child==node->child(dir)) {
            if (((node->version.load()^nodeV)&IGNOREGROW) != 0)
                return -1;
            void* p = attemptSearch(k, child, nextD, chV);
            if (p!=-1)
                return p;
        }
    }
}

bool AVLTreeLF::insert(int k, int v) {
    return (bool)attemptPut(k, v, rootHolder, 1, 0);
}

int AVLTreeLF::attemptInsert(int k, int v, NodeLF* node, int dir, long nodeV) {
    int p = -1; // Retry represented as -1
    do {
        NodeLF* child = node->child(dir);
        if (((node->version.load()^nodeV)&IGNOREGROW) != 0)
            return -1; // Retry
        if (child==nullptr) {
            p = attemptAdd(k, v, node, dir, nodeV);
        } else {
            int nextD = (k<child->key) ? -1 : (k>child->key) ? 1 : 0;
            if (nextD==0) {
                return 0; // Failure, key already in tree
            } else {
                long chV = child->version.load();
                if ((chV&SHRINKING) != 0) {
                    waitUntilNotChanging(child);
                } else if (chV!=UNLINKED && child==node->child(dir)) {
                    if (((node->version.load()^nodeV)&IGNOREGROW) != 0)
                        return -1;
                    p = attemptInsert(k, v, child, nextD, chV);
                }
            }
        }
    } while (p == -1);
    return p;
}

int AVLTreeLF::attemptAdd(int k, int v, NodeLF* node, int dir, long nodeV) {
    NodeLF* expectedChild = nullptr;
    if ((node->version.load()^nodeV)&IGNOREGROW || node->child(dir)!=nullptr)
        return -1;
    if (node->child(dir)==nullptr) {
        NodeLF* newNode = new NodeLF(k, v, node, 0, nullptr, nullptr);
        std::atomic<NodeLF*>& child = (dir==1) ? node->right : node->left;
        if (child.compare_exchange_strong(expectedChild, newNode)) { // Successful insertion
            fixHeightAndRebalance(node);
            return 1;
        } else {
            delete newNode;
            return -1;
        }
    }
    return -1;
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


void AVLTreeLF::waitUntilNotChanging(Node n) {
    long v = n.version;
    if ((v & (Growing | Shrinking)) != 0) {
        int i = 0;
        while (n.version == v && i < SPINCOUNT) ++i;
        if (i == SPINCOUNT) synchronized (n) { };
    }
}

NodeLF::NodeLF(int k) : key(k), value(0), parent(nullptr), left(nullptr), right(nullptr), height(1), version(0) {}

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
