#include <bits/stdc++.h>
#include "coarsegrained.h"
using namespace std;

// Coarse-grained: IMPL=1, fine-grained: IMPL=2, lock-free: IMPL=3
int IMPL;

AVLTreeCG *treeCG;

/* UTILITY FUNCTIONS */
void printImpl() {
    if (IMPL == 1) printf("Coarse-Grained AVL Tree\n");
}

void initTree() {
    if (IMPL == 1) treeCG = new AVLTreeCG();
}

void deleteTree() {
    if (IMPL == 1) delete treeCG;
}

std::vector<int> getBlockVector(int low, int high) {
    std::vector<int> v(high - low);
    std::iota(v.begin(), v.end(), low);
    return v;
}

std::vector<int> getShuffledVector(int low, int high) {
    std::vector<int> v(high - low);
    std::iota(v.begin(), v.end(), low);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);
    return v;
}

void flexInsert(int k) {
    if (IMPL==1) treeCG->insert(k);
}

void flexDelete(int k) {
    if (IMPL==1) treeCG->deleteNode(k);
}

void flexSearch(int k) {
    if (IMPL==1) treeCG->search(k);
}

/* HELPER FUNCTIONS */
void insertRange(int low, int high, std::vector<int> keyVector) {
    for (int i = low; i < min(high, (int)keyVector.size()); i++) {
        flexInsert(keyVector[i]);
    }
}

void deleteRange(int low, int high, std::vector<int> keyVector) {
    for (int i = low; i < min(high, (int)keyVector.size()); i++) {
        flexDelete(keyVector[i]);
    }
}

void searchRange(int low, int high, std::vector<int> keyVector) {
    for (int i = low; i < min(high, (int)keyVector.size()); i++) {
        flexSearch(keyVector[i]);
    }
}

double parallelInsert(int capacity, int numThreads, std::vector<int> &keyVector) {
    std::vector<thread> threads;
    const auto startTime = std::chrono::steady_clock::now();
    // Launch threads for insertion
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(std::thread(insertRange, i * capacity, (i + 1) * capacity, std::ref(keyVector)));
    }
    // Wait for threads to finish
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
    // Calculate time taken
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - startTime).count();
}

double parallelDelete(int capacity, int numThreads, std::vector<int> &keyVector) {
    std::vector<thread> threads;
    const auto startTime = std::chrono::steady_clock::now();
    // Launch threads for deletion
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(std::thread(deleteRange, i * capacity, (i + 1) * capacity, std::ref(keyVector)));
    }
    // Wait for threads to finish
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
    // Calculate time taken
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - startTime).count();
}

double parallelSearch(int capacity, int numThreads, std::vector<int> &keyVector) {
    std::vector<thread> threads;
    const auto startTime = std::chrono::steady_clock::now();
    // Launch threads for search
    for (int i = 0; i < numThreads; i++) {
        threads.push_back(std::thread(searchRange, i * capacity, (i + 1) * capacity, std::ref(keyVector)));
    }
    // Wait for threads to finish
    for (int i = 0; i < numThreads; i++) {
        threads[i].join();
    }
    // Calculate time taken
    return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - startTime).count();
}


/* TEST FUNCTIONS */
void testContiguousInsert(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads*threadCapacity);
    const double computeTime = parallelInsert(threadCapacity, numThreads, keyVector);
    // printf("Contiguous insert for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Contiguous insert for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

void testRandomInsert(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelInsert(threadCapacity, numThreads, keyVector);
    // printf("Random insert for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Random insert for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

void testContiguousDeleteContiguousTree(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);
    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    // printf("Contiguous delete on contiguous tree for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Contiguous delete on contiguous tree for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

void testContiguousDeleteRandomTree(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);
    keyVector = getBlockVector(0, numThreads * threadCapacity);
    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    // printf("Contiguous delete on random tree for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Contiguous delete on random tree for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

void testRandomDeleteContiguousTree(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);
    keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    // printf("Random delete on contiguous tree for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Random delete on contiguous tree for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

void testRandomDeleteRandomTree(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    // printf("Random delete on random tree for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Random delete on random tree for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

void testContiguousSearch(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);
    const double computeTime = parallelSearch(threadCapacity, numThreads, keyVector);
    // printf("Random search for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Random search for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

void testRandomSearch(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelSearch(threadCapacity, numThreads, keyVector);
    // printf("Random search for %d capacity and %d threads: %f milliseconds, %f operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / computeTime);
    outFile << "Random search for " << threadCapacity << " capacity and " << numThreads << " threads: " << computeTime << " milliseconds, " << threadCapacity * numThreads / computeTime << " operations per millisecond\n";
    deleteTree();
}

// (insert, delete, search) = (50%, 30%, 20%)
void testRandom1(int numThreads, int threadCapacity, ofstream& outFile) {
    initTree();
    // All operations
    std::vector<int> keyVector = getShuffledVector(0, numThreads * threadCapacity);

    // Ratio
    int numInsert = int(keyVector.size() * 0.5); // 50% insert
    int numDelete = int(keyVector.size() * 0.3); // 30% delete
    // int numSearch = int(keyVector.size() * 0.2); // 20% search 

    // Measure time for insert operations
    vector<int> insertKeys(keyVector.begin(), keyVector.begin() + numInsert);
    double insertTime = parallelInsert(threadCapacity, numThreads, insertKeys);

    // Measure time for delete operations
    vector<int> deleteKeys(keyVector.begin() + numInsert, keyVector.begin() + numInsert + numDelete);
    double deleteTime = parallelDelete(threadCapacity, numThreads, deleteKeys);

    // Measure time for search operations
    vector<int> searchKeys(keyVector.begin() + numInsert + numDelete, keyVector.end());
    double searchTime = parallelSearch(threadCapacity, numThreads, searchKeys);

    // Calculate overall throughput
    double totalTime = insertTime + deleteTime + searchTime;
    double throughput = (keyVector.size()) / (totalTime); // Operations per milisecond
    
    // Output results to file
    outFile << "Insert time: " << insertTime << " milliseconds" << endl;
    outFile << "Delete time: " << deleteTime << " milliseconds" << endl;
    outFile << "Search time: " << searchTime << " milliseconds" << endl;
    outFile << "Total time: " << totalTime << " milliseconds" << endl;
    outFile << "Total throughput: " << throughput << " operations per second" << endl;
    outFile << endl;
}

/* MAIN FUNCTION */
int main(int argc, char const *argv[]) {
    std::vector<int> numThreads = {1, 2, 4, 8, 16, 32, 64, 128};
    std::vector<int> threadCapacities = {1000000, 100000, 10000};
    std::vector<int> impl = {1, 2, 3};

    // Throughput
    for (int m : impl) {
        IMPL = m;
        printImpl();

        // Open a file for writing results
        // Constructing the file path
        std::string filePath = "./result/throughput_results_" + std::to_string(IMPL) + ".txt";

        // Opening the file
        std::ofstream outFile(filePath);

        if (!outFile.is_open()) {
            cerr << "Error: Could not open the file." << endl;
            return 1;
        }

        for (int capacity : threadCapacities) {
            for (int threads : numThreads) {
                // // testContiguousInsert(threads, capacity/threads, outFile);
                // testRandomInsert(threads, capacity/threads, outFile);
                // // testContiguousDeleteContiguousTree(threads, capacity/threads, outFile);
                // // testContiguousDeleteRandomTree(threads, capacity/threads, outFile);
                // // testRandomDeleteContiguousTree(threads, capacity/threads, outFile);
                // testRandomDeleteRandomTree(threads, capacity/threads, outFile);
                // // testContiguousSearch(threads, capacity/threads, outFile);
                // testRandomSearch(threads, capacity/threads, outFile);
                outFile << "Implementation: " << m << ", Capacity: " << capacity / threads << ", Threads: " << threads << endl;
                testRandom1(threads, capacity/threads, outFile);
            }
        }

        // Close the file
        outFile.close();
    }
    return 0;
}