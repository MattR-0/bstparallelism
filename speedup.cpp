#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include "sequential.h"
#include "coarsegrained.h"
#include <chrono>
#include <random> // Include the <random> header for random number generation

#define CAPACITY 10000  // Capacity of the AVL trees
#define IMPL 1

int main() {
    // Constructing the file path
    std::string filePath = "./result/speedup_results_" + std::to_string(IMPL) + ".txt";

    // Opening the file
    std::ofstream outputFile(filePath);    
    
    int num_threads_list[] = {1, 2, 4, 8, 16, 32, 64, 128};
    std::random_device rd; // Obtain a random number from hardware
    std::mt19937 eng(rd()); // Seed the generator
    std::uniform_int_distribution<> distr(0, CAPACITY - 1); // Define the range

    // Sequential AVL Tree
    AVLTree* avl_tree = new AVLTree();
    auto start_seq_insert = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < CAPACITY; ++i)
        avl_tree->insert(distr(eng)); // Insert a random number
    auto end_seq_insert = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seq_insert = end_seq_insert - start_seq_insert;

    auto start_seq_delete = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < CAPACITY; ++i)
        avl_tree->deleteNode(distr(eng)); // Delete a random number
    auto end_seq_delete = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seq_delete = end_seq_delete - start_seq_delete;

    auto start_seq_search = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < CAPACITY; ++i)
        avl_tree->search(distr(eng)); // Search for a random number
    auto end_seq_search = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seq_search = end_seq_search - start_seq_search;

    for (int num_threads : num_threads_list) {
        // Coarse-Grained AVL Tree
        AVLTreeCG* avl_tree_cg = new AVLTreeCG();
        auto start_cg_insert = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads_insert;
        for (int i = 0; i < num_threads; ++i) {
            threads_insert.emplace_back([&avl_tree_cg, &distr, &eng, num_threads]() {
                for (int j = 0; j < CAPACITY / num_threads; ++j)
                    avl_tree_cg->insert(distr(eng)); // Search for random numbers
            });
        }
        for (auto& thread : threads_insert)
            thread.join();
        auto end_cg_insert = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_cg_insert = end_cg_insert - start_cg_insert;

        auto start_cg_delete = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads_delete;
        for (int i = 0; i < num_threads; ++i) {
            threads_delete.emplace_back([&avl_tree_cg, &distr, &eng, num_threads]() {
                for (int j = 0; j < CAPACITY / num_threads; ++j)
                    avl_tree_cg->deleteNode(distr(eng)); // Search for random numbers
            });
        }
        for (auto& thread : threads_delete)
            thread.join();
        auto end_cg_delete = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_cg_delete = end_cg_delete - start_cg_delete;

        auto start_cg_search = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads_search;
        for (int i = 0; i < num_threads; ++i) {
            threads_search.emplace_back([&avl_tree_cg, &distr, &eng, num_threads]() {
                for (int j = 0; j < CAPACITY / num_threads; ++j)
                    avl_tree_cg->search(distr(eng)); // Search for random numbers
            });
        }
        for (auto& thread : threads_search)
            thread.join();
        auto end_cg_search = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_cg_search = end_cg_search - start_cg_search;

        // Calculate speedup
        double insertion_speedup = elapsed_seq_insert.count() / elapsed_cg_insert.count();
        double deletion_speedup = elapsed_seq_delete.count() / elapsed_cg_delete.count();
        double search_speedup = elapsed_seq_search.count() / elapsed_cg_search.count();

        // Output speedup to file
        outputFile << "Threads: " << num_threads << ", Operations: " << CAPACITY << ","
            << "Insertion Speedup: " << insertion_speedup << ", Insertion Time (Sequential): " << elapsed_seq_insert.count() * 1000 << " milliseconds, Insertion Time (Concurrent): " << elapsed_cg_insert.count() * 1000 << " milliseconds,"
            << "Deletion Speedup: " << deletion_speedup << ", Deletion Time (Sequential): " << elapsed_seq_delete.count() * 1000 << " milliseconds, Deletion Time (Concurrent): " << elapsed_cg_delete.count() * 1000 << " milliseconds,"
            << "Search Speedup: " << search_speedup << ", Search Time (Sequential): " << elapsed_seq_search.count() * 1000 << " milliseconds, Search Time (Concurrent): " << elapsed_cg_search.count() * 1000 << " milliseconds\n";

    }

    std::cout << "Speedup results written to 'speedup_results.txt'\n";
    outputFile.close();
    return 0;
}
