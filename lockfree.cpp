#include <lockfree.h>

#define FLAG_MASK 3UL
#define NULL_MASK 1UL

// Operation marking status
enum opFlag {
    NONE, MARK, ROTATE, INSERT
};

Operation* AVLTreeLF::flag(Operation* op, int status) {
    (Operation*) ((uintptr_t)op | status);
}

Operation* AVLTreeLF::getFlag(Operation* op) {
    (Operation*) ((uintptr_t)op & FLAG_MASK);
}

Operation* AVLTreeLF::unflag(Operation* op) {
    return (Operation*)(((uintptr_t)op >> 2) << 2);
}

bool AVLTreeLF::search(int k, NodeLF* node) {
    bool result = false;
    while (node!=nullptr) {
        if (k<node->key)
            node = node->left;
        else if (k>node->key)
            node = node->right;
        else {
            result = true;
            break;
        }
    }
    return result;
}

int AVLTreeLF::seek(int key, NodeLF** parent, Operation** parentOp, NodeLF** node, Operation** nodeOp, NodeLF* auxRoot, NodeLF* root) {
    NodeLF* next;
    int result;
    bool retry = false;
    while (true) {
        result = -1;
        *node = auxRoot;
        *nodeOp = (*node)->op;
        if (getFlag(*nodeOp)!=NONE && auxRoot==root) {
            bstHelpInsert(unflag(*nodeOp), *node);
            continue;
        }
        next = (*node)->right;
        while (next!=nullptr) {
            *parent = *node
            *parentOp = *nodeOp;
            *node = next;
            *nodeOp = (*node)->op;
            if (getFlag(nodeOp) != NONE) {
                help(*parent, *parentOp, *node, *nodeOp);
                retry = true;
                break;
            }
            int nodeKey = (*node)->key;
            if (key<nodeKey) {
                next = (*node)->left;
                result = -1;
            } else if (key>nodeKey) {
                next = (*node)->right;
                result = 1;
            } else
                return 0;
        }
        if (!retry)
            return result;
        else
            retry = false;
    }
    return result;
}

bool AVLTreeLF::insert(int key, NodeLF* root) {
    NodeLF* parent;
    NodeLF* current;
    NodeLF* newNode = nullptr;
    Operation* parentOp;
    Operation* currentOp;
    Operation* casOp;
    int result = 0;

    while (true) {
        result = seek(key, &parent, &parentOp, &current, &currentOp, root, root);
        if (result==0)
            return false;
        if (newNode==nullptr)
            newNode = new NodeLF(key);
        bool isLeft = (result==-1);
        NodeLF* oldNode = isLeft ? current->left : current->right;
        casOp = new InsertOp(isLeft, oldNode, newNode);
        if (__sync_bool_compare_and_swap(current->op, currentOp, flag(casOp, INSERT)) {
            helpInsert(casOp, current);
            return true;
        }
    }
}
