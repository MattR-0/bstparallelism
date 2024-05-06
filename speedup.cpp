#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <random> // Include the <random> header for random number generation

#include "sequential.h"
#include "coarsegrained.h"
#include "finegrained.h"
// #include "lockfree2.h"
#include "lockfree.h"

#define CAPACITY 100000  // Capacity of the AVL trees
#define IMPL 1

int main() {
    // Constructing the file path
    std::string filePath = "./result/speedup_results_" + std::to_string(IMPL) + "_" + std::to_string(CAPACITY) + ".txt";

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

    if (IMPL == 1) {
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
    }

    if (IMPL == 2) {
         for (int num_threads : num_threads_list) {
            // Coarse-Grained AVL Tree
            AVLTreeFG* avl_tree_fg = new AVLTreeFG();
            auto start_fg_insert = std::chrono::high_resolution_clock::now();
            std::vector<std::thread> threads_insert;
            for (int i = 0; i < num_threads; ++i) {
                threads_insert.emplace_back([&avl_tree_fg, &distr, &eng, num_threads]() {
                    for (int j = 0; j < CAPACITY / num_threads; ++j)
                        avl_tree_fg->insert(distr(eng)); // Search for random numbers
                });
            }
            for (auto& thread : threads_insert)
                thread.join();
            auto end_fg_insert = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed_fg_insert = end_fg_insert - start_fg_insert;

            auto start_fg_delete = std::chrono::high_resolution_clock::now();
            std::vector<std::thread> threads_delete;
            for (int i = 0; i < num_threads; ++i) {
                threads_delete.emplace_back([&avl_tree_fg, &distr, &eng, num_threads]() {
                    for (int j = 0; j < CAPACITY / num_threads; ++j)
                        avl_tree_fg->deleteNode(distr(eng)); // Search for random numbers
                });
            }
            for (auto& thread : threads_delete)
                thread.join();
            auto end_fg_delete = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed_fg_delete = end_fg_delete - start_fg_delete;

            auto start_fg_search = std::chrono::high_resolution_clock::now();
            std::vector<std::thread> threads_search;
            for (int i = 0; i < num_threads; ++i) {
                threads_search.emplace_back([&avl_tree_fg, &distr, &eng, num_threads]() {
                    for (int j = 0; j < CAPACITY / num_threads; ++j)
                        avl_tree_fg->search(distr(eng)); // Search for random numbers
                });
            }
            for (auto& thread : threads_search)
                thread.join();
            auto end_fg_search = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed_fg_search = end_fg_search - start_fg_search;

            // Calculate speedup
            double insertion_speedup = elapsed_seq_insert.count() / elapsed_fg_insert.count();
            double deletion_speedup = elapsed_seq_delete.count() / elapsed_fg_delete.count();
            double search_speedup = elapsed_seq_search.count() / elapsed_fg_search.count();

            // Output speedup to file
            outputFile << "Threads: " << num_threads << ", Operations: " << CAPACITY << ","
                << "Insertion Speedup: " << insertion_speedup << ", Insertion Time (Sequential): " << elapsed_seq_insert.count() * 1000 << " milliseconds, Insertion Time (Concurrent): " << elapsed_fg_insert.count() * 1000 << " milliseconds,"
                << "Deletion Speedup: " << deletion_speedup << ", Deletion Time (Sequential): " << elapsed_seq_delete.count() * 1000 << " milliseconds, Deletion Time (Concurrent): " << elapsed_fg_delete.count() * 1000 << " milliseconds,"
                << "Search Speedup: " << search_speedup << ", Search Time (Sequential): " << elapsed_seq_search.count() * 1000 << " milliseconds, Search Time (Concurrent): " << elapsed_fg_search.count() * 1000 << " milliseconds\n";
    
        }
        std::cout << "Speedup results written to 'speedup_results.txt'\n";
    }
    if (IMPL == 4) {
        for (int num_threads : num_threads_list) {
            // Coarse-Grained AVL Tree
            AVLTreeLF *bst_tree = new AVLTreeLF();
            auto start_bst_insert = std::chrono::high_resolution_clock::now();
            std::vector<std::thread> threads_insert;
            for (int i = 0; i < num_threads; ++i) {
                threads_insert.emplace_back([&bst_tree, &distr, &eng, num_threads]() {
                    for (int j = 0; j < CAPACITY / num_threads; ++j)
                        bst_tree->insert(distr(eng)); // Search for random numbers
                });
            }
            for (auto& thread : threads_insert)
                thread.join();
            auto end_bst_insert = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed_bst_insert = end_bst_insert - start_bst_insert;

            auto start_bst_delete = std::chrono::high_resolution_clock::now();
            std::vector<std::thread> threads_delete;
            for (int i = 0; i < num_threads; ++i) {
                threads_delete.emplace_back([&bst_tree, &distr, &eng, num_threads]() {
                    for (int j = 0; j < CAPACITY / num_threads; ++j)
                        bst_tree->deleteNode(distr(eng)); // Search for random numbers
                });
            }
            for (auto& thread : threads_delete)
                thread.join();
            auto end_bst_delete = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed_bst_delete = end_bst_delete - start_bst_delete;

            auto start_bst_search = std::chrono::high_resolution_clock::now();
            std::vector<std::thread> threads_search;
            for (int i = 0; i < num_threads; ++i) {
                threads_search.emplace_back([&bst_tree, &distr, &eng, num_threads]() {
                    for (int j = 0; j < CAPACITY / num_threads; ++j)
                        bst_tree->search(distr(eng)); // Search for random numbers
                });
            }
            for (auto& thread : threads_search)
                thread.join();
            auto end_bst_search = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed_bst_search = end_bst_search - start_bst_search;

            // Calculate speedup
            double insertion_speedup = elapsed_seq_insert.count() / elapsed_bst_insert.count();
            double deletion_speedup = elapsed_seq_delete.count() / elapsed_bst_delete.count();
            double search_speedup = elapsed_seq_search.count() / elapsed_bst_search.count();

            // Output speedup to file
            outputFile << "Threads: " << num_threads << ", Operations: " << CAPACITY << ","
                << "Insertion Speedup: " << insertion_speedup << ", Insertion Time (Sequential): " << elapsed_seq_insert.count() * 1000 << " milliseconds, Insertion Time (Concurrent): " << elapsed_bst_insert.count() * 1000 << " milliseconds,"
                << "Deletion Speedup: " << deletion_speedup << ", Deletion Time (Sequential): " << elapsed_seq_delete.count() * 1000 << " milliseconds, Deletion Time (Concurrent): " << elapsed_bst_delete.count() * 1000 << " milliseconds,"
                << "Search Speedup: " << search_speedup << ", Search Time (Sequential): " << elapsed_seq_search.count() * 1000 << " milliseconds, Search Time (Concurrent): " << elapsed_bst_search.count() * 1000 << " milliseconds\n";
    
        }
        std::cout << "Speedup results written to 'speedup_results.txt'\n";
    }

    outputFile.close();
    return 0;
}
