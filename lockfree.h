class Operation {

};

class NodeBST {
public:
    int volatile key;
    Operation* volatile op;
    NodeBST* volatile left;
    NodeBST* volatile right;

    NodeBST();
    NodeBST(int k);
};

class ChildCASOp : public Operation {
public:
    bool is_left;
    NodeBST* expected;
    NodeBST* update;

    ChildCASOp(bool is_left, NodeBST* old_n, NodeBST* new_n);
};

class RelocateOp : public Operation {
public:
    int volatile state = 0;
    NodeBST* dest;
    Operation* dest_op;
    int key_to_remove;
    int key_to_put;

    RelocateOp(NodeBST* curr, Operation* curr_op, int curr_key, int new_key);
};

class AVLTreeLF {
public:
    NodeBST root;
    AVLTreeLF();
    ~AVLTreeLF();

    bool insert(int key);
    bool deleteNode(int key);
    bool search(int key);

private:
    int find(int k, NodeBST*& parent, Operation*& parent_op, NodeBST*& curr, Operation*& curr_op, NodeBST* root);
    void help(NodeBST* parent, Operation* parent_op, NodeBST* curr, Operation* curr_op);
    void helpMarked(NodeBST* parent, Operation* parent_op, NodeBST* curr);
    void helpChildCAS(Operation* op, NodeBST* dest);
    bool helpRelocate(Operation* op, NodeBST* parent, Operation* parentOp, NodeBST* curr);
};
