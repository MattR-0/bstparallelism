#include "lockfree2.h"
#include <limits.h>

Node::Node(int k, int v) {
    ver.setInitVal(0);
    key.setInitVal(k);
    left.setInitVal(NULL);
    right.setInitVal(NULL);
    parent.setInitVal(NULL);
    height.setInitVal(1);
    val.setInitVal(v);
}
AVLTree::AVLTree() {
    Node* maxNode = new Node(INT_MAX, 0);
    Node* minNode = new Node(INT_MIN, 0);
    maxRoot->left = minRoot;
    minNode->parent = maxRoot;
    maxRoot.setInitVal(maxNode);
    minRoot.setInitVal(minNode);
}
AVLTree::~AVLTree() {}

bool AVLTree::search(int k) {
    auto [n, nvers, p, pvers, res] = searchHelper(k);
    return res;
}

std::tuple<Node*, uint64_t, Node*, uint64_t, bool> AVLTree::searchHelper(int key) {
    while (true) { //Retry loop
        std::vector<Node*> path; // Node* vector
        std::vector<uint64_t> vers; // int vector
        path.push_back(maxRoot);
        vers.push_back(maxRoot->ver); // Note: uses KCASRead
        Node* n = maxRoot->left; // Note: uses KCASRead
        size_t sz = 1; // Number of nodes in path
        int predIx = -1;
        int succIx = 0; // Index of k’s pred/succ in path
        while (true) {
            if (n==NULL) // Reached a leaf
                if (validatePath(path, vers, sz)) {
                    int a = std::min(predIx, succIx); // The shallower of pred/succ (ancestor)
                    return std::make_tuple(path[a], vers[a], path[sz - 1], vers[sz - 1], false);
                } else
                    break; // Failed validation
            path.push_back(n);
            vers.push_back(n->ver); // Note: uses KCASRead
            int currKey = n->key; // Note: uses KCASRead
            sz++;
            if (key>currKey) {
                predIx = sz - 1;
                n = n->right; // Note: uses KCASRead
            } else if (key<currKey) {
                succIx = sz - 1;
                n = n->left; // Note: uses KCASRead
            } else
                return std::make_tuple(path[sz - 1], vers[sz - 1], path[sz - 2], vers[sz - 2], true);
        }
    }
}

bool AVLTree::validatePath(std::vector<Node*> path, std::vector<uint64_t> vers, size_t sz) {
    for (size_t i = 0; i<sz; i++) {
        if (path[i]->ver!=vers[i] || isMarked(vers[i]))
            return false;
    }
    return true;
}

bool AVLTree::insert(int k) {
    return insertIfAbsent(k, 0);
}

bool AVLTree::insertIfAbsent(int k, int v) {
    while (true) {
        auto [a, aVer, p, pVer, res] = searchHelper(k);
        if (res)
            return false;
        kcas::start();
        Node* n = new Node(k, v);
        if (k > p->key)
            kcas::add(&p->right, (Node*)NULL, n);
        else if (k < p->key)
            kcas::add(&p->left, (Node*)NULL, n);
        else
            continue;
        kcas::add(&a->ver, aVer, aVer);
        kcas::add(&p->ver, pVer, pVer+2);
        if (kcas::execute()) {
            rebalance(p);
            return true;
        }
    }
}

bool AVLTree::isMarked(uint64_t version) {
    return (version&1) == 1;
}

bool AVLTree::deleteNode(int k) {
    return erase(k);
}

bool AVLTree::erase(int k) {
    while (true) {
        auto [n, nVer, p, pVer, res] = searchHelper(k);
        if (!res)
            return false;
        if (isMarked(pVer) || isMarked(nVer))
            continue;
        Node* l = n->left;
        Node* r = n->right;
        if (l==NULL || r==NULL)
            res = eraseSimple(k, n, nVer, p, pVer);
        else
            res = eraseTwoChild(n, nVer, p, pVer);
        if (res)
            return true;
    }
}

bool AVLTree::eraseTwoChild(Node* n, uint64_t nVer, Node* p, uint64_t pVer) {
    kcas::start();
    auto [s, sVer, sp, spVer, ret] = getSuccessor(n);
    int nVal = n->val;
    int sVal = s->val;
    int nKey = n->key;
    int sKey = s->key;
    if (!ret || isMarked(sVer) || isMarked(spVer))
        return false;
    Node* sr = s->right;
    if (sr!=NULL) {
        uint64_t srVer = sr->ver;
        if (isMarked(srVer))
            return false;
        kcas::add(&sr->parent, s, sp);
        kcas::add(&sr->ver, srVer, srVer+2);
    }
    if (sp->right==s)
        kcas::add(&sp->right, s, sr);
    else if (sp->left==s)
        kcas::add(&sp->left, s, sr);
    else
        return false;
    kcas::add(&n->val, nVal, sVal);
    kcas::add(&n->key, nKey, sKey);
    kcas::add(&s->ver, sVer, sVer+1);
    kcas::add(&sp->ver, spVer, spVer+2);
    if (sp!=n) // n and sp can be the same Node
        kcas::add(&n->ver, nVer, nVer+2);
    if (kcas::execute()) {
        rebalance(sp);
        return true;
    }
    return false;
}

bool AVLTree::eraseSimple(int key, Node* n, uint64_t nVer, Node* p, uint64_t pVer) {
    kcas::start();
    Node* r;
    if (n->left!=NULL)
        r = n->left;
    else if (n->right!=NULL)
        r = n->right;
    else
        r = NULL;
    if (r!=NULL) { // n has one child
        uint64_t rVer = r->ver;
        if (isMarked(rVer))
            return false;
        kcas::add(&r->parent, n, p);
        kcas::add(&r->ver, rVer, rVer+2);
    }
    if (p->right==n)
        kcas::add(&p->right, n, r);
    else if (p->left==n)
        kcas::add(&p->left, n, r);
    else
        return false;
    kcas::add(&p->ver, pVer, pVer+2);
    kcas::add(&n->ver, nVer, nVer+1);
    if (kcas::execute()) {
        rebalance(p);
        return true;
    }
    return false;
}

std::tuple<Node*, uint64_t, Node*, uint64_t, bool> AVLTree::getSuccessor(Node* n) {
    Node* parent = n;
    Node* succ = n->right;
    if (!succ) return std::make_tuple(nullptr, 0, nullptr, 0, false);
    uint64_t parentVer = parent->ver;
    uint64_t succVer = succ->ver;
    while (succ->left != nullptr) {
        parent = succ;
        parentVer = succVer;
        succ = succ->left;
        succVer = succ->ver;
    }
    if (isMarked(succVer) || isMarked(parentVer)) {
        return std::make_tuple(nullptr, 0, nullptr, 0, false);
    }
    return std::make_tuple(succ, succVer, parent, parentVer, true);
}

void AVLTree::rebalance(Node* n) {
    Node* p;
    Node* l;
    Node* r;
    Node* ll;
    Node* lr;
    Node* rr;
    Node* rl;
    uint64_t nVer, pVer, lVer, rVer, llVer, lrVer, rrVer, rlVer;
    int lh, llh, lrh, rh, rrh, rlh;
    int nBalance, lBalance, rBalance;
    while (n!=minRoot) {
        nVer = n->ver;
        if (isMarked(nVer))
            return;
        p = n->parent;
        pVer = p->ver;
        if (isMarked(pVer))
            continue;
        l = n->left;
        if (l!=NULL)
            lVer = l->ver;
        r = n->right;
        if (r!=NULL)
            rVer = r->ver;
        if (isMarked(lVer) || isMarked(rVer))
            continue;
        lh = (l==NULL ? 0 : l->height); rh = (r==NULL ? 0 : r->height);
        nBalance = lh - rh;
        if (nBalance>=2) {
            ll = l->left;
            if (ll!=NULL)
                llVer = ll->ver;
            lr = l->right;
            if (lr!=NULL)
                lrVer = lr->ver;
            if (isMarked(llVer) || isMarked(lrVer))
                continue;
            llh = (ll==NULL ? 0 : ll->height);
            lrh = (lr==NULL ? 0 : lr->height);
            lBalance = llh - lrh;
            if (lBalance<0)
                if (rotateLeftRight(p, pVer, n, nVer, l, lVer, r, rVer, lr, lrVer)) {
                    rebalance(n);
                    rebalance(l);
                    rebalance(lr);
                    n = p;
                } else if (rotateRight(p, pVer, n, nVer, l, lVer, r, rVer)) {
                    rebalance(n);
                    rebalance(l);
                    n = p;
                }
        } else if (nBalance<=-2) {
            rr = r->right;
            if (rr!=NULL)
                rrVer = rr->ver;
            rl = r->left;
            if (rl!=NULL)
                rlVer = rl->ver;
            if (isMarked(rrVer) || isMarked(rlVer))
                continue;
            rrh = (rr==NULL ? 0 : rr->height);
            rlh = (rl==NULL ? 0 : rl->height);
            rBalance = rrh - rlh;
            if (rBalance<0)
                if (rotateRightLeft(p, pVer, n, nVer, l, lVer, r, rVer, rl, rlVer)) {
                    rebalance(n);
                    rebalance(r);
                    rebalance(rl);
                    n = p;
                } else if (rotateLeft(p, pVer, n, nVer, l, lVer, r, rVer)) {
                    rebalance(n);
                    rebalance(r);
                    n = p;
                }
        } else {
            int res = fixHeight(n, nVer);
            if (res==0)
                continue;
            else if (res==1)
                n = n->parent;
            else
                return;
        }
    }
}

int AVLTree::fixHeight(Node* n, uint64_t nVer) {
    kcas::start();
    Node* l = n->left;
    Node* r = n->right;
    uint64_t lVer;
    uint64_t rVer;
    if (l!=NULL) {
        lVer = l->ver;
        kcas::add(&l->ver, lVer, lVer);
    }
    if (r!=NULL) {
        rVer = r->ver;
        kcas::add(&l->ver, rVer, rVer);
    }
    int oldHeight = n->height;
    int newHeight = 1 + (max(l->height.getValue(), r->height.getValue()));
    if (oldHeight==newHeight) // If the height is correct, no need to update
        if (n->ver==nVer && (l==NULL || l->ver==lVer) && (r==NULL || r->ver==rVer))
            return -1; // UNNECESSARY
    else
        return 0; // FAILURE
    kcas::add(&n->height, oldHeight, newHeight, &n->ver, nVer, nVer + 2);
    if (kcas::execute())
         return 1; // SUCCESS
    return 0; // FAILURE
}

bool AVLTree::rotateRight(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer) {
    kcas::start();
    if (p->right==n)
        kcas::add(&p->right, n, l);
    else if (p->left==n)
        kcas::add(&p->left, n, l);
    else
        return false;
    Node* lr = l->right;
    int lrHeight = 0;
    if (lr!=NULL) {
        uint64_t lrVer = lr->ver;
        lrHeight = lr->height;
        if (isMarked(lrVer))
            return false;
        kcas::add(&lr->parent, l, n);
        kcas::add(&lr->ver, lrVer, lrVer + 2);
    }
    Node* ll = l->left;
    int llHeight = 0;
    if (ll!=NULL) {
        uint64_t llVer = ll->ver;
        llHeight = ll->height;
        if (isMarked(llVer))
            return false;
        kcas::add(&ll->ver, llVer, llVer);
    }
    int rHeight = 0;
    if (r!=NULL) {
        uint64_t rVer = r->ver;
        rHeight = r->height;
        kcas::add(&r->ver, rVer, rVer);
    }
    int oldNHeight = n->height;
    int oldLHeight = l->height;
    int newNHeight = 1 + max(lrHeight, rHeight);
    int newLHeight = 1 + max(llHeight, newNHeight);
    kcas::add(&l->parent, n, p);
    kcas::add(&n->left, l, lr);
    kcas::add(&l->right, lr, n);
    kcas::add(&n->parent, p, l);
    kcas::add(&n->height, oldNHeight, newNHeight);
    kcas::add(&l->height, oldLHeight, newLHeight);
    kcas::add(&p->ver, pVer, pVer + 2);
    kcas::add(&n->ver, nVer, nVer + 2);
    kcas::add(&l->ver, lVer, lVer + 2);
    if (kcas::execute())
        return true;
    return false;
}

bool AVLTree::rotateLeft(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer) {
    kcas::start();
    if (p->left==n)
        kcas::add(&p->left, n, l);
    else if (p->right==n)
        kcas::add(&p->right, n, l);
    else
        return false;
    Node* rl = r->left;
    int rlHeight = 0;
    if (rl!=NULL) {
        uint64_t rlVer = rl->ver;
        rlHeight = rl->height;
        if (isMarked(rlVer))
            return false;
        kcas::add(&rl->parent, l, n, &rl->ver, rlVer, rlVer + 2);
    }
    Node* rr = r->right;
    int rrHeight = 0;
    if (rr!=NULL) {
        uint64_t rrVer = rr->ver;
        rrHeight = rr->height;
        if (isMarked(rrVer))
            return false;
        kcas::add(&rr->ver, rrVer, rrVer);
    }
    int lHeight = 0;
    if (l!=NULL) {
        uint64_t lVer = l->ver;
        lHeight = l->height;
        kcas::add(&l->ver, lVer, lVer);
    }
    int oldNHeight = n->height;
    int oldRHeight = r->height;
    int newNHeight = 1 + max(rlHeight, lHeight);
    int newRHeight = 1 + max(rrHeight, newNHeight);
    kcas::add(&r->parent, n, p);
    kcas::add(&n->right, r, rl);
    kcas::add(&r->left, rl, n);
    kcas::add(&n->parent, p, r);
    kcas::add(&n->height, oldNHeight, newNHeight);
    kcas::add(&r->height, oldRHeight, newRHeight);
    kcas::add(&p->ver, pVer, pVer + 2);
    kcas::add(&n->ver, nVer, nVer + 2);
    kcas::add(&r->ver, rVer, rVer + 2);
    if (kcas::execute())
        return true;
    return false;
}

bool AVLTree::rotateLeftRight(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer, Node* lr, uint64_t lrVer) {
    kcas::start();
    if (p->right==n)
        kcas::add(&p->right, n, lr);
    else if (p->left==n)
        kcas::add(&p->left, n, lr);
    else
        return false;
    Node* lrl = lr->left;
    int lrlHeight = 0;
    if (lrl!=NULL) {
        uint64_t lrlVer = lrl->ver;
        lrlHeight = lrl->height;
        if (isMarked(lrlVer))
            return false;
        kcas::add(&lrl->parent, lr, l, &lrl->ver, lrlVer, lrlVer + 2);
    }
    Node* lrr = lr->right;
    int lrrHeight = 0;
    if (lrr!=NULL) {
        uint64_t lrrVer = lrr->ver;
        lrrHeight = lrr->height;
        if (isMarked(lrrVer))
            return false;
        kcas::add(&lrr->parent, lr, n, &lrr->ver, lrrVer, lrrVer + 2);
    }
    int rHeight = 0;
    if (r!=NULL) {
        uint64_t rVer = r->ver;
        rHeight = r->height;
        kcas::add(&r->ver, rVer, rVer);
    }
    Node* ll = l->left;
    int llHeight = 0;
    if (ll!=NULL) {
        uint64_t llVer = ll->ver;
        llHeight = ll->height;
        if (isMarked(llVer))
            return false;
        kcas::add(&ll->ver, llVer, llVer);
    }
    int oldNHeight = n->height;
    int oldLHeight = l->height;
    int oldLRHeight = lr->height;
    int newNHeight = 1 + max(lrrHeight, rHeight);
    int newLHeight = 1 + max(llHeight, lrlHeight);
    int newLRHeight = 1 + max(newNHeight, newLHeight);
    kcas::add(&lr->parent, l, p);
    kcas::add(&lr->left, lrl, l);
    kcas::add(&l->parent, n, lr);
    kcas::add(&lr->right, lrr, n);
    kcas::add(&n->parent, p, lr);
    kcas::add(&l->right, lr, lrl);
    kcas::add(&n->left, l, lrr);
    kcas::add(&n->height, oldNHeight, newNHeight);
    kcas::add(&l->height, oldLHeight, newLHeight);
    kcas::add(&lr->height, oldLRHeight, newLRHeight);
    kcas::add(&lr->ver, lrVer, lrVer + 2);
    kcas::add(&p->ver, pVer, pVer + 2);
    kcas::add(&n->ver, nVer, nVer + 2);
    kcas::add(&l->ver, lVer, lVer + 2);
    if (kcas::execute())
        return true;
    return false;
}

bool AVLTree::rotateRightLeft(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer, Node* rl, uint64_t rlVer) {
    kcas::start();
    if (p->left==n)
        kcas::add(&p->left, n, rl);
    else if (p->right==n)
        kcas::add(&p->right, n, rl);
    else
        return false;
    Node* rlr = rl->right;
    int rlrHeight = 0;
    if (rlr!=NULL) {
        uint64_t rlrVer = rlr->ver;
        rlrHeight = rlr->height;
        if (isMarked(rlrVer))
            return false;
        kcas::add(&rlr->parent, rl, r, &rlr->ver, rlrVer, rlrVer + 2);
    }
    Node* rll = rl->left;
    int rllHeight = 0;
    if (rll!=NULL) {
        uint64_t rllVer = rll->ver;
        rllHeight = rll->height;
        if (isMarked(rllVer))
            return false;
        kcas::add(&rll->parent, rl, n, &rll->ver, rllVer, rllVer + 2);
    }
    int lHeight = 0;
    if (l!=NULL) {
        uint64_t lVer = l->ver;
        lHeight = l->height;
        kcas::add(&l->ver, lVer, lVer);
    }
    Node* rr = r->right;
    int rrHeight = 0;
    if (rr!=NULL) {
        uint64_t rrVer = rr->ver;
        rrHeight = rr->height;
        if (isMarked(rrVer))
            return false;
        kcas::add(&rr->ver, rrVer, rrVer);
    }
    int oldNHeight = n->height;
    int oldRHeight = r->height;
    int oldRLHeight = rl->height;
    int newNHeight = 1 + max(rllHeight, lHeight);
    int newRHeight = 1 + max(rrHeight, rlrHeight);
    int newRLHeight = 1 + max(newNHeight, newRHeight);
    kcas::add(&rl->parent, r, p);
    kcas::add(&rl->right, rlr, r);
    kcas::add(&r->parent, n, rl);
    kcas::add(&rl->left, rll, n);
    kcas::add(&n->parent, p, rl);
    kcas::add(&r->left, rl, rlr);
    kcas::add(&n->right, r, rll);
    kcas::add(&n->height, oldNHeight, newNHeight);
    kcas::add(&r->height, oldRHeight, newRHeight);
    kcas::add(&rl->height, oldRLHeight, newRLHeight);
    kcas::add(&rl->ver, rlVer, rlVer + 2);
    kcas::add(&p->ver, pVer, pVer + 2);
    kcas::add(&n->ver, nVer, nVer + 2);
    kcas::add(&r->ver, rVer, rVer + 2);
    if (kcas::execute())
        return true;
    return false;
}
