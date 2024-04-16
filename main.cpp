#include "coarsegrained.h"
#include <fstream>
#include <getopt.h>

// Operation read from file
struct Operation {
    std::string op;
    int key;
};

int main(int argc, char *argv[]) {
    std::string input_filename;
    int num_threads = 0;
    int opt;

    while ((opt = getopt(argc, argv, "f:n:")) != -1) {
        switch (opt) {
        case 'f':
            input_filename = optarg;
            break;
        case 'n':
            num_threads = std::stoi(optarg);
            break;
        default:
            std::cerr << "Usage: " << argv[0] << " -f input_filename -n num_threads\n";
            exit(EXIT_FAILURE);
        }
    }

    if (input_filename.empty() || num_threads <= 0) {
        std::cerr << "Invalid arguments\n";
        exit(EXIT_FAILURE);
    }

    std::ifstream fin(input_filename);
    if (!fin) {
        std::cerr << "Unable to open file: " << input_filename << "\n";
        exit(EXIT_FAILURE);
    }

    omp_set_num_threads(num_threads);
    AVLTree tree;
    std::string op;
    int key;
    std::vector<Operation> ops;
    while (fin >> op >> key) {
        Operation opStruct{op, key};
        ops.push_back(opStruct);
    }
    #pragma omp parallel for
    for (auto& inputOp : ops) {
        if (inputOp.op == "insert")
            tree.root = tree.insert(tree.root, inputOp.key);
        else if (inputOp.op == "delete")
            tree.root = tree.deleteNode(tree.root, inputOp.key);
        else if (inputOp.op == "search") {
            bool found = tree.search(tree.root, inputOp.key);
            if (found)
                std::cout << inputOp.key << " is present in the tree.\n";
            else
                std::cout << inputOp.key << " is not present in the tree.\n";
        } else if (inputOp.op == "preorder")
            tree.preOrder(tree.root);
        else
            std::cerr << "Invalid operation: " << inputOp.op << "\n";
    }

    tree.preOrder(tree.root);
    std::cout << "\n";

    return 0;
}
