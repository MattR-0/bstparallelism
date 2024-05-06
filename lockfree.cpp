#include "lockfree.h"
#include <stdint.h>
#include <stdio.h>

#define FLAG_MASK 3UL
#define NULL_MASK 1UL
#define GET_FLAG(op) ((uintptr_t)op & FLAG_MASK)
#define SET_FLAG(op, flag) ((Operation*) ((uintptr_t)op | flag))
#define DE_FLAG(op) ((Operation*) (((uintptr_t)op >> 2) << 2))
#define IS_NULL(node) (((uintptr_t) node & NULL_MASK) == 1UL)
#define SET_NULL(node) ((NodeBST*) ((uintptr_t) node | NULL_MASK))

enum op_flag {
    NONE, MARK, CHILDCAS, RELOCATE
};

enum findVals {
    FOUND, NOTFOUND_L, NOTFOUND_R, ABORT
};

enum relocateVals {
    ONGOING, SUCCEED, FAILED
};

NodeBST::NodeBST() : key(0) {
    op = NULL;
    left = NULL;
    right = NULL;
    op = SET_FLAG(op, NONE);
    left = SET_NULL(left);
    right = SET_NULL(right);
}

NodeBST::NodeBST(int k) : key(k) {
    op = NULL;
    left = NULL;
    right = NULL;
    op = SET_FLAG(op, NONE);
    left = SET_NULL(left);
    right = SET_NULL(right);
}

AVLTreeLF::AVLTreeLF() {
    root = *(new NodeBST(-1));
}

AVLTreeLF::~AVLTreeLF() {

}

ChildCASOp::ChildCASOp(bool is_left, NodeBST* old, NodeBST* new_n) {
    this->is_left = is_left;
    expected = old;
    update = new_n;
}

RelocateOp::RelocateOp(NodeBST* curr, Operation* curr_op, int curr_key, int new_key) {
    state = ONGOING;
    dest = curr;
    dest_op = curr_op;
    key_to_remove = curr_key;
    key_to_put = new_key;
}

int AVLTreeLF::find(int k, NodeBST*& parent, Operation*& parent_op, NodeBST*& curr, Operation*& curr_op, NodeBST* root) {
    int result, current_key;
    NodeBST* next, *last_right;
    Operation* last_right_op;
    bool is_occupied = false;

    while (true) {
        result = NOTFOUND_R;
        curr = root;
        curr_op = curr->op;
        if (GET_FLAG(curr_op) != NONE) {
            if (root == &this->root) {
                helpChildCAS(DE_FLAG(curr_op), curr);
                continue;
            }
            else return ABORT;
        }
        next = curr->right;
        last_right = curr;
        last_right_op = curr_op;

        while (!IS_NULL(next)) {
            parent = curr;
            parent_op = curr_op;
            curr = next;
            curr_op = next->op;
            if (GET_FLAG(curr_op) != NONE) {
                help(parent, parent_op, curr, curr_op);
                is_occupied = true;
                break;
            }
            current_key = curr->key;
            if (k < current_key) {
                result = NOTFOUND_L;
                next = curr->left;
            }
            else if (k > current_key) {
                result = NOTFOUND_R;
                next = curr->right;
                last_right = curr;
                last_right_op = curr_op;
            }
            else {
                result = FOUND;
                break;
            }
        }

        if (is_occupied) {
            is_occupied = false;
            continue;
        }
        if ((result == FOUND || last_right_op == last_right->op) && curr_op == curr->op) break;
    }
    return result;
}

bool AVLTreeLF::search(int k) {
    NodeBST* parent, *curr;
    Operation* parent_op, *curr_op;
    return find(k, parent, parent_op, curr, curr_op, &root) == FOUND;
}

bool AVLTreeLF::insert(int k) {
    NodeBST* parent, *curr, *new_node;
    Operation* parent_op, *curr_op, *cas_op;
    int result;

    while(true) {
        result = find(k, parent, parent_op, curr, curr_op, &root);
        if (result == FOUND)
            return false;
        new_node = new NodeBST(k);
        bool isLeft = (result == NOTFOUND_L);
        NodeBST* old = isLeft ? curr->left : curr->right;
        cas_op = new ChildCASOp(isLeft, old, new_node);
        if (__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG(cas_op, CHILDCAS))) {
            helpChildCAS(cas_op, curr);
            return true;
        }
    }
}

bool AVLTreeLF::deleteNode(int k) {
    NodeBST* parent, *curr, *replace;
    Operation* parent_op, *curr_op, *replace_op, *relocate_op;

    while (true) {
        if (find(k, parent, parent_op, curr, curr_op, &root) != FOUND)
            return false;

        if (IS_NULL(curr->left) || IS_NULL(curr->right)) {
            if (__sync_bool_compare_and_swap(&curr->op, curr_op, SET_FLAG(curr_op, MARK))) {
                helpMarked(parent, parent_op, curr);
                return true;
            }
        }
        else {
            if (find(k, parent, parent_op, replace, replace_op, curr) == ABORT || (curr->op != curr_op))
                continue;
            relocate_op = new RelocateOp(curr, curr_op, k, replace->key);
            if (__sync_bool_compare_and_swap(&replace->op, replace_op, SET_FLAG(relocate_op, RELOCATE))) {
                if (helpRelocate(relocate_op, parent, parent_op, replace))
                    return true;
            }
        }
    }
}

void AVLTreeLF::help(NodeBST* parent, Operation* parent_op, NodeBST* curr, Operation* curr_op) {
    if (GET_FLAG(curr_op) == CHILDCAS)
        helpChildCAS(DE_FLAG(curr_op), curr);
    else if (GET_FLAG(curr_op) == RELOCATE)
        helpRelocate(DE_FLAG(curr_op), parent, parent_op, curr);
    else if (GET_FLAG(curr_op) == MARK)
        helpMarked(parent, parent_op, curr);
}

void AVLTreeLF::helpMarked(NodeBST* parent, Operation* parent_op, NodeBST* curr) {
    NodeBST* tmp;
    if (IS_NULL(curr->left)) {
        if (IS_NULL(curr->right)) tmp = SET_NULL(curr);
        else tmp = curr->right;
    }
    else tmp = curr->left;

    bool is_left = (curr == parent->left);
    Operation* cas_op = new ChildCASOp(is_left, curr, tmp);

    if (__sync_bool_compare_and_swap(&parent->op, parent_op, SET_FLAG(cas_op, CHILDCAS))) {
        helpChildCAS(cas_op, parent);
    }
}

void AVLTreeLF::helpChildCAS(Operation* op, NodeBST* dest) {
    ChildCASOp* child_op = (ChildCASOp *) op;
    NodeBST* volatile* addr = child_op->is_left ? &dest->left : &dest->right;
    __sync_bool_compare_and_swap(addr, child_op->expected, child_op->update);
    __sync_bool_compare_and_swap(&dest->op, SET_FLAG(op, CHILDCAS), SET_FLAG(op, NONE));
}

bool AVLTreeLF::helpRelocate(Operation* op, NodeBST* parent, Operation* parent_op, NodeBST* curr) {
    RelocateOp* relo_op = (RelocateOp *)op;
    bool result;
    int state = relo_op->state;

    if (state == ONGOING) {
        __sync_bool_compare_and_swap(&relo_op->dest->op, relo_op->dest_op, SET_FLAG(relo_op, RELOCATE));
        Operation* seen_op = relo_op->dest->op;
        if ((seen_op == relo_op->dest_op) || (seen_op == SET_FLAG(relo_op, RELOCATE))) {
            __sync_bool_compare_and_swap(&relo_op->state, ONGOING, SUCCEED);
            state = SUCCEED;
        }
        else {
            __sync_bool_compare_and_swap(&relo_op->state, ONGOING, FAILED);
            state = relo_op->state;
        }
    }
    if (state == SUCCEED) {
        __sync_bool_compare_and_swap(&relo_op->dest->key, relo_op->key_to_remove, relo_op->key_to_put);
        __sync_bool_compare_and_swap(&relo_op->dest->op, SET_FLAG(relo_op, RELOCATE), SET_FLAG(relo_op, NONE));
    }

    result = (state == SUCCEED);
    if (relo_op->dest == curr) return result;

    __sync_bool_compare_and_swap(&curr->op, SET_FLAG(relo_op, RELOCATE), SET_FLAG(relo_op, result ? MARK : NONE));
    if (result) {
        if (relo_op->dest == parent) parent_op = SET_FLAG(relo_op, NONE);
        helpMarked(parent, parent_op, curr);
    }
    return result;
}
