#include "kcasfull.h"

class Node {
public:
    casword<uint64_t> ver;
    casword<int> key;
    casword<Node*> left;
    casword<Node*> right;
    casword<Node*> parent;
    casword<int> height;
    casword<int> val;
    
    Node(casword<Node*> p, casword<int> k, casword<int> v);
};

class AVLTree {
public:
    casword<Node*> maxRoot;
    casword<Node*> minRoot;
    
    bool search(int k);
    bool insert(int k);
    bool deleteNode(int k);

    AVLTree();
    ~AVLTree();
private:
    std::tuple<casword<Node*>, casword<int>, casword<Node*>, casword<int>, bool> search(casword<int> key);
    bool validatePath(std::vector<casword<Node*>> path, std::vector<casword<int>> vers, size_t sz);
    bool insertIfAbsent(casword<int> k, casword<int> val);
    bool erase(casword<int> k);
    bool eraseTwoChild(casword<Node*> n, casword<int> nVer, casword<Node*> p, casword<int> pVer);
    bool eraseSimple(casword<int> key, casword<Node*> n, casword<int> nVer, casword<Node*> p, casword<int> pVer);
    void rebalance(casword<Node*> n);
    int fixHeight(casword<Node*> n, casword<int> nVer);
    bool rotateRight(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer);
    bool rotateLeft(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer);
    bool rotateLeftRight(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer, casword<Node*> lr, casword<int> lrVer);
    bool rotateRightLeft(casword<Node*> p, casword<int> pVer, casword<Node*> n, casword<int> nVer, casword<Node*> l, casword<int> lVer, casword<Node*> r, casword<int> rVer, casword<Node*> rl, casword<int> rlVer);
};
