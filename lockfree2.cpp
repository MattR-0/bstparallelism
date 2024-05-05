#include "lockfree2.h"
#include <limits.h>

Node::Node(Node* p, int k, int v) {
    ver.setInitVal(0);
    key.setInitVal(k);
    left.setInitVal(NULL);
    right.setInitVal(NULL);
    parent.setInitVal(p);
    height.setInitVal(1);
    val.setInitVal(v);
}
AVLTree::AVLTree() {
    Node* maxNode = new Node(NULL, INT_MAX, 0);
    Node* minNode = new Node(NULL, INT_MIN, 0);
    maxRoot->left = minRoot;
    minNode->parent = maxRoot;
    maxRoot.setInitVal(maxNode);
    minRoot.setInitVal(minNode);
}
AVLTree::~AVLTree() {}

bool AVLTree::search(int k) {
    auto [n, nvers, p, pvers, res] = searchHelper((casword<int>)k);
    return res;
}

std::tuple<casword<Node*>, casword<int>, casword<Node*>, casword<int>, bool> AVLTree::searchHelper(casword<int> key) {
    while (true) { //Retry loop
        std::vector<casword<Node*>> path; // casword<Node*> vector
        std::vector<casword<int>> vers; // casword<int> vector
        path.push_back(maxRoot);
        vers.push_back(kcas::readVal(maxRoot->ver)); // Note: uses KCASRead
        casword<Node*> n = kcas::readPtr(maxRoot->left); // Note: uses KCASRead
        size_t sz = 1; // Number of nodes in path
        int predIx = -1;
        int succIx = 0; // Index of k’s pred/succ in path
        while (true) {
            if (n==NULL) // Reached a leaf
                if validatePath(path, vers, sz) {
                    int a = std::min(predIx, succIx); // The shallower of pred/succ (ancestor)
                    return std::make_tuple(path[a], vers[a], path[sz - 1], vers[sz - 1], false);
                } else
                    break; // Failed validation
            path.push_back(n);
            vers.push_back(kcas::readVal(n->ver)); // Note: uses KCASRead
            casword<int> currKey = kcas::readVal(n->key); // Note: uses KCASRead
            sz++;
            if (key>currKey) {
                predIx = sz - 1;
                n = kcas::readPtr(n->right); // Note: uses KCASRead
            } else if (key<currKey) {
                succIx = sz - 1;
                n = kcas::readPtr(n->left); // Note: uses KCASRead
            } else
                return std::make_tuple(path[sz - 1], vers[sz - 1], path[sz - 2], vers[sz - 2], true);
        }
    }
}

bool AVLTree::validatePath(std::vector<casword<Node*>> path, std::vector<casword<int>> vers, size_t sz) {
    for (size_t i = 0; i<sz; i++) {
        if (path[i]->ver!=ver[i] || isMarked(ver[i]))
            return false;
    }
    return true;
}

bool AVLTree::insert(int k) {
    return insertIfAbsent((casword<int>)k, (casword<int>)0);
}

bool AVLTree::insertIfAbsent(casword<int> k, casword<int> val) {
    while (true) {
        auto [a, aVer, p, pVer, res] = searchHelper(k)
        if (res)
            return false;
        kcas::start();
        casword<Node*> n = new Node(p, k, v);
        if (k > p->key)
            kcas::add(&p->right, NULL, n);
        else if (k < p->key)
            kcas::add(&p->left, NULL, n);
        else
            continue;
        kcas::add(&a->ver, aVer, aVer, &p->ver, pVer, pVer+2);
        if kcas::execute() {
            rebalance(p);
            return true;
        }
    }
}

bool AVLTree::deleteNode(int k) {
    return erase((casword<int>)k);
}

bool AVLTree::erase(casword<int> k) {
    while (true) {
        auto [n, nVer, p, pVer, res] = searchHelper(k);
        if (!res)
            return false;
        if (isMarked(pVer) || isMarked(nVer))
            continue;
        casword<Node*> l = n->left;
        casword<Node*> r = n->right;
        if (l==NULL || r==NULL)
            res = eraseSimple(k, n, nVer, p, pVer);
        else
            res = eraseTwoChild(n, nVer, p, pVer);
        if (res)
            return true;
    }
}

bool AVLTree::eraseTwoChild(casword<Node*> n, casword<int> nVer, casword<Node*> p, casword<int> pVer) {
    kcas::start()
    auto [hs, sVer, sp, spVer, reti] = getSuccessor(n);
    if (!ret || isMarked(sVer) || isMarked(spVer))
        return false;
    casword<Node*> sr = s->right;
    if (sr!=NULL) {
        srVer = sr->ver;
        if (isMarked(srVer))
            return false;
        kcas::add(&sr->parent, s, sp, &sr->ver, srVer, srVer+2);
    }
    if (sp->right==s)
        kcas::add(&sp->right, s, sr);
    else if (sp->left==s)
        kcas::add(&sp->left, s, sr);
    else
        return false;
    kcas::add(&n->val, n->val, s->val, &n->key, key, s->key, &s->ver, sVer, sVer+1, &sp->ver, spVer, spVer+2);
    if (sp!=n) // n and sp can be the same Node
        kcas::add(&n->ver, nVer, nVer+2);
    if (kcas::execute()) {
        rebalance(sp);
        return true;
    }
    return false;
}

bool AVLTree::eraseSimple(casword<int> key, casword<Node*> n, casword<int> nVer, casword<Node*> p, casword<int> pVer) {
    kcas::start();
    casword<Node*> r;
    if (n->left!=NULL)
        r = n->left;
    else if (n->right!=NULL)
        r = n->right;
    else
        r = NULL;
    if (r!=NULL) { // n has one child
        rVer = r->ver;
        if (isMarked(rVer))
            return false;
        kcas::add(&r->parent, n, p, &r->ver, rVer, rVer+2);
    }
    if (p->right==n)
        kcas::add(&p->right, n, r);
    else if (p->left==n)
        kcas::add(&p->left, n, r);
    else
        return false;
    kcas::add(&p->ver, pVer, pVer+2, &n->ver, nVer, nVer+1);
    if kcas::execute() {
        rebalance(p);
        return true;
    }
    return false;
}

void AVLTree::rebalance(casword<Node*> n) {
    casword<Node*> p;
    casword<Node*> l;
    casword<Node*> r;
    casword<Node*> ll;
    casword<Node*> lr;
    casword<Node*> rr;
    casword<Node*> rl;
    casword<int> pVer;
    casword<int> lVer;
    casword<int> rVer;
    casword<int> llVer;
    casword<int> lrVer;
    casword<int> rrVer;
    casword<int> rlVer;
    casword<int> lh;
    casword<int> llh;
    casword<int> lrh;
    casword<int> rh;
    casword<int> rrh;
    casword<int> rlh;
    casword<int> nBalance;
    casword<int> lBalance;
    casword<int> rBalance;
    while (n!=minRoot) {
        nVer = n->ver;
        if (isMarked(nVer))
            return
        p = n->parent;
        pVer = p->ver;
        if (isMarked(pVer))
            continue;
        l = n->left;
        if (l!=NULL)
            lVer = l->ver;
        r = n->right
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

int AVLTree::fixHeight(casword<Node*> n, casword<int> nVer) {
    kcas::start();
    Node* l = n->left;
    Node* r = n->right;
    if (l!=NULL) {
        int lVer = l->ver;
        kcas::add(&l->ver, lVer, lVer);
    }
    if (r!=NULL) {
        int rVer = r->ver;
        kcas::add(&l->ver, rVer, rVer);
    }
    oldHeight = n->height;
    newHeight = 1 + max(l->height, r->height);
    if (oldHeight==newHeight) // If the height is correct, no need to update
        if (n->ver==nVer && (l==NULL || l->ver==lVer) && (r==NULL || r->ver==rVer))
            return -1 // UNNECESSARY
    else
        return 0 // FAILURE
    kcas::add(&n->height, oldHeight, newHeight,
    &n->ver, nVer, nVer + 2)
    if kcas::execute()
         return 1 // SUCCESS
    return 0 // FAILURE
}

bool AVLTree::rotateRight(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer) {
    kcas::start()
    if (p->right=n)
        kcas::add(&p->right, n, l);
    else if (p->left=n)
        kcas::add(&p->left, n, l);
    else
        return false;
    Node* lr = l->right;
    int lrHeight = 0;
    if (lr!=NULL) {
        int lrVer = lr->ver;
        lrHeight = lr->height;
        if isMarked(lrVer)
            return false;
        kcas::add(&lr->parent, l, n, &lr->ver, lrVer, lrVer + 2);
    }
    Node* ll = l->left;
    int llHeight = 0;
    if (ll!=NULL) {
        int llVer = ll->ver;
        llHeight = ll->height;
        if isMarked(llVer)
            return false;
        kcas::add(&ll->ver, llVer, llVer);
    }
    int rHeight = 0;
    if (r!=NULL) {
        int rVer = r->ver;
        rHeight = r->height;
        kcas::add(&r->ver, rVer, rVer);
    }
    int oldNHeight = n->height;
    int oldLHeight = l->height;
    int newNHeight = 1 + max(lrHeight, rHeight);
    int newLHeight = 1 + max(llHeight, newNHeight);
    kcas::add(&l->parent, n, p,
              &n->left, l, lr,
              &l->right, lr, n,
              &n->parent, p, l,
              &n->height, oldNHeight, newNHeight,
              &l->height, oldLHeight, newLHeight,
              &p->ver, pVer, pVer + 2,
              &n->ver, nVer, nVer + 2,
              &l->ver, lVer, lVer + 2);
    if kcas::execute()
        return true;
    return false;
}

bool AVLTree::rotateLeft(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer) {
    kcas::start()
    if (p->left=n)
        kcas::add(&p->left, n, l);
    else if (p->right=n)
        kcas::add(&p->right, n, l);
    else
        return false;
    Node* rl = r->left;
    int rlHeight = 0;
    if (rl!=NULL) {
        int rlVer = rl->ver;
        rlHeight = rl->height;
        if isMarked(rlVer)
            return false;
        kcas::add(&rl->parent, l, n, &rl->ver, rlVer, rlVer + 2);
    }
    Node* rr = r->right;
    int rrHeight = 0;
    if (rr!=NULL) {
        int rrVer = rr->ver;
        rrHeight = rr->height;
        if isMarked(rrVer)
            return false;
        kcas::add(&rr->ver, rrVer, rrVer);
    }
    int lHeight = 0;
    if (l!=NULL) {
        int lVer = l->ver;
        lHeight = l->height;
        kcas::add(&l->ver, lVer, lVer);
    }
    int oldNHeight = n->height;
    int oldRHeight = r->height;
    int newNHeight = 1 + max(rlHeight, lHeight);
    int newRHeight = 1 + max(rrHeight, newNHeight);
    kcas::add(&r->parent, n, p,
              &n->right, r, rl,
              &r->left, rl, n,
              &n->parent, p, r,
              &n->height, oldNHeight, newNHeight,
              &r->height, oldRHeight, newRHeight,
              &p->ver, pVer, pVer + 2,
              &n->ver, nVer, nVer + 2,
              &r->ver, rVer, rVer + 2);
    if kcas::execute()
        return true;
    return false;
}

bool AVLTree::rotateLeftRight(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer, casword<Node*> lr, casword<int> lrVer) {
    kcas::start()
    if (p->right==n)
        kcas::add(&p->right, n, lr);
    else if (p->left==n)
        kcas::add(&p->left, n, lr);
    else
        return false;
    casword<Node*> lrl = lr->left;
    int lrlHeight = 0;
    if (lrl!=NULL) {
        casword<int> lrlVer = lrl->ver;
        lrlHeight = lrl->height;
        if (isMarked(lrlVer))
            return false;
        kcas::add(&lrl->parent, lr, l, &lrl->ver, lrlVer, lrlVer + 2);
    }
    casword<Node*> lrr = lr->right;
    int lrrHeight = 0;
    if (lrr!=NULL) {
        casword<int> lrrVer = lrr->ver;
        lrrHeight = lrr->height;
        if (isMarked(lrrVer))
            return false;
        kcas::add(&lrr->parent, lr, n, &lrr->ver, lrrVer, lrrVer + 2)
    }
    int rHeight = 0;
    if (r!=NULL) {
        casword<int> rVer = r->ver;
        rHeight = r->height;
        kcas::add(&r->ver, rVer, rVer)
    }
    Node* ll = l->left;
    int llHeight = 0;
    if (ll!=NULL) {
        casword<int> llVer = ll->ver;
        llHeight = ll->height;
        if (isMarked(llVer))
            return false;
        kcas::add(&ll->ver, llVer, llVer);
    }
    oldNHeight = n->height;
    oldLHeight = l->height;
    oldLRHeight = lr->height;
    newNHeight = 1 + max(lrrHeight, rHeight);
    newLHeight = 1 + max(llHeight, lrlHeight);
    newLRHeight = 1 + max(newNHeight, newLHeight);
    kcas::add(&lr->parent, l, p,
              &lr->left, lrl, l,
              &l->parent, n, lr,
              &lr->right, lrr, n,
              &n->parent, p, lr,
              &l->right, lr, lrl,
              &n->left, l, lrr,
              &n->height, oldNHeight, newNHeight,
              &l->height, oldLHeight, newLHeight,
              &lr->height, oldLRHeight, newLRHeight,
              &lr->ver, lrVer, lrVer + 2,
              &p->ver, pVer, pVer + 2,
              &n->ver, nVer, nVer + 2,
              &l->ver, lVer, lVer + 2);
    if (kcas::execute())
        return true;
    return false;
}

bool AVLTree::rotateRightLeft(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer, casword<Node*> rl, casword<int> rlVer) {
    kcas::start();
    if (p->left==n)
        kcas::add(&p->left, n, rl);
    else if (p->right==n)
        kcas::add(&p->right, n, rl);
    else
        return false;
    casword<Node*> rlr = rl->right;
    int rlrHeight = 0;
    if (rlr!=NULL) {
        casword<int> rlrVer = rlr->ver;
        rlrHeight = rlr->height;
        if (isMarked(rlrVer))
            return false;
        kcas::add(&rlr->parent, rl, r, &rlr->ver, rlrVer, rlrVer + 2);
    }
    casword<Node*> rll = rl->left;
    int rllHeight = 0;
    if (rll!=NULL) {
        casword<int> rllVer = rll->ver;
        rllHeight = rll->height;
        if (isMarked(rllVer))
            return false;
        kcas::add(&rll->parent, rl, n, &rll->ver, rllVer, rllVer + 2);
    }
    int lHeight = 0;
    if (l!=NULL) {
        casword<int> lVer = l->ver;
        lHeight = l->height;
        kcas::add(&l->ver, lVer, lVer);
    }
    Node* rr = r->right;
    int rrHeight = 0;
    if (rr!=NULL) {
        casword<int> rrVer = rr->ver;
        rrHeight = rr->height;
        if (isMarked(rrVer))
            return false;
        kcas::add(&rr->ver, rrVer, rrVer);
    }
    oldNHeight = n->height;
    oldRHeight = r->height;
    oldRLHeight = rl->height;
    newNHeight = 1 + max(rllHeight, lHeight);
    newRHeight = 1 + max(rrHeight, rlrHeight);
    newRLHeight = 1 + max(newNHeight, newRHeight);
    kcas::add(&rl->parent, r, p,
              &rl->right, rlr, r,
              &r->parent, n, rl,
              &rl->left, rll, n,
              &n->parent, p, rl,
              &r->left, rl, rlr,
              &n->right, r, rll,
              &n->height, oldNHeight, newNHeight,
              &r->height, oldRHeight, newRHeight,
              &rl->height, oldRLHeight, newRLHeight,
              &rl->ver, rlVer, rlVer + 2,
              &p->ver, pVer, pVer + 2,
              &n->ver, nVer, nVer + 2,
              &r->ver, rVer, rVer + 2);
    if (kcas::execute())
        return true;
    return false;
}
