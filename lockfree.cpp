#include <lockfree.h>

struct NodeT {
    int key;
    NodeT* left;
    NodeT* right;
    OperationT* op;
    int localHeight;
    int lh;
    int rh;
    bool deleted;
    bool removed;
};

union OperationT {
    InsertOpT insertOp;
    RotateOpT rotateOp;
};

struct InsertOpT {
    bool isLeft;
    bool isUpdate = false;
    NodeT* expected;
    NodeT* newN;
};

struct RotateOpT {
    volatile int state = UNDECIDED;
    NodeT* parent;
    NodeT* node;
    NodeT* child;
    OperationT* pOp;
    OperationT* nOp;
    OperationT* cOp;
    bool rightR;
    bool dir;
};

// Operation marking status
#define NONE 0
#define MARK 1
#define ROTATE 2
#define INSERT 3

// Input: int k, NodeT* root
// Output: true, if the key is found in the set otherwise false
bool search(int k, NodeT* node) {
    bool result = false;
    while (node!=nullptr) {
        int nodeKey = node->key;
        if (k<nodeKey)
            node = node->left;
        else if (k>nodeKey)
            node = node->right;
        else {
            result = true;
            break;
        }
    }
    if (result && node->deleted) {
        if (getFlag(node->op)==INSERT) {
            if (node->op->insertOp.newN->key==k) {
                return true;
            }
        }
        return false;
    }
    return result;
}

int seek(int key, NodeT** parent, OperationT** parentOp, NodeT** node, OperationT** nodeOp, NodeT* auxRoot, NodeT* root) {
    int result = -1;
    while (true) {
        *node = auxRoot;
        *nodeOp = (*node)->op;
        if (getFlag(*nodeOp)!=NONE && auxRoot==root) {
            bstHelpInsert((OperationT*)unflag(*nodeOp), *node);
            continue;
        }
        *node = (*node)->right;
        while (*node!=nullptr) {
            *parent = *node;
            *parentOp = *nodeOp;
            int nodeKey = (*node)->key;
            if (key<nodeKey) {
                *node = (*node)->left;
                result = -1;
            } else if (key>nodeKey) {
                *node = (*node)->right;
                result = 1;
            } else
                return 0;
        }
        if (getFlag(*nodeOp) != NONE)
            help(*parent, *parentOp, *node, *nodeOp);
        else
            return result;
    }
}

bool insert(int key, NodeT* root) {
    NodeT* parent;
    NodeT* current;
    NodeT* newNode = nullptr;
    OperationT* parentOp;
    OperationT* nodeOp;
    OperationT* casOp;
    int result = 0;

    while (true) {
        result = seek(key, &parent, &parentOp, &current, &nodeOp, root, root);
        if (result==0 && !current->deleted)
            return false;
        if (newNode==nullptr)
            newNode = new NodeT{key, nullptr, nullptr, nodeOp, 1, 0, 0, false, false};
        NodeT* old = (result==-1) ? current->left : current->right;
        casOp = new OperationT{new InsertOpT, new RotateOpT};
        if (result==0 && current->deleted)
            casOp->insertOp.isUpdate = true;
        casOp->insertOp.isLeft = (result==-1);
        casOp->insertOp.expected = old;
        casOp->insertOp.newNode = newNode;
        if (current->op.compare_exchange_strong(nodeOp, FLAG(insertOp, INSERT)) {
            helpInsert(casOp, current);
            return true;
        }
    }
}
