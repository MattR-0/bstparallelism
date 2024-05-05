#include "kcasfull.h"
#include <vector>

class Node {
public:
    casword<uint64_t> ver;
    casword<int> key;
    casword<Node*> left;
    casword<Node*> right;
    casword<Node*> parent;
    casword<int> height;
    casword<int> val;
    
    Node(int k, int v);
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
    std::tuple<Node*, uint64_t, Node*, uint64_t, bool> searchHelper(int key);
    bool validatePath(std::vector<Node*> path, std::vector<uint64_t> vers, size_t sz);
    bool insertIfAbsent(int k, int val);
    bool isMarked(uint64_t ver);
    bool erase(int k);
    bool eraseTwoChild(Node* n, uint64_t nVer, Node* p, uint64_t pVer);
    bool eraseSimple(int key, Node* n, uint64_t nVer, Node* p, uint64_t pVer);
    std::tuple<Node*, uint64_t, Node*, uint64_t, bool> getSuccessor(Node* n);
    void rebalance(Node* n);
    int fixHeight(Node* n, uint64_t nVer);
    bool rotateRight(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer);
    bool rotateLeft(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer);
    bool rotateLeftRight(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer, Node* lr, uint64_t lrVer);
    bool rotateRightLeft(Node* p, uint64_t pVer, Node* n, uint64_t nVer, Node* l, uint64_t lVer, Node* r, uint64_t rVer, Node* rl, uint64_t rlVer);
};
