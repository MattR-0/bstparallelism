#include "coarsegrained.h"
#include <fstream>
#include <getopt.h>

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

    AVLTree tree;
    std::string op;
    int key;
    while (fin >> op >> key) {
        if (op == "insert") {
            tree.insert(tree.root, key);
        } else if (op == "delete") {
            tree.deleteNode(tree.root, key);
        } else if (op == "search") {
            bool found = tree.search(tree.root, key);
            std::cout << key << (found ? " is present" : " is not present") << " in the tree.\n";
        } else if (op == "preorder") {
            tree.preOrder(tree.root);
        } else {
            std::cerr << "Invalid operation: " << op << "\n";
        }
    }

    tree.preOrder(tree.root);
    std::cout << "\n";

    return 0;
}
