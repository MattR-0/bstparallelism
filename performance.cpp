#include <bits/stdc++.h>
#include "coarsegrained.h"
using namespace std;
using namespace std::chrono;

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
    for (int i=low; i<high; i++) {
        flexInsert(keyVector[i]);
    }
}

void deleteRange(int low, int high, std::vector<int> keyVector) {
    for (int i=low; i<high; i++) {
        flexDelete(keyVector[i]);
    }
}

void searchRange(int low, int high, std::vector<int> keyVector) {
    for (int i=low; i<high; i++) {
        flexSearch(keyVector[i]);
    }
}

double parallelInsert(int capacity, int numThreads, std::vector<int> &keyVector) {
    std::vector<thread> threads;
    const double startTime = std::chrono::steady_clock::now();
    for (int i=0; i<numThreads; i++) {
        threads.push_back(thread(insertRange, i*capacity, (i+1)*capacity, keyVector));
    }
    const double computeTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - startTime).count();
    for (int i=0; i<numThreads; i++) {
        threads[i].join();
    }
    return computeTime;
}

double parallelDelete(int capacity, int numThreads, std::vector<int> &keyVector) {
    std::vector<thread> threads;
    const double startTime = std::chrono::steady_clock::now();
    for (int i=0; i<numThreads; i++) {
        threads.push_back(thread(deleteRange, i*capacity, (i+1)*capacity, keyVector));
    }
    const double computeTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - startTime).count();
    for (int i=0; i<numThreads; i++) {
        threads[i].join();
    }
    return computeTime;
}

double parallelSearch(int capacity, int numThreads, std::vector<int> &keyVector) {
    std::vector<thread> threads;
    const double startTime = std::chrono::steady_clock::now();
    for (int i=0; i<numThreads; i++) {
        threads.push_back(thread(searchRange, i*capacity, (i+1)*capacity, keyVector));
    }
    const double computeTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - startTime).count();
    for (int i=0; i<numThreads; i++) {
        threads[i].join();
    }
    return computeTime;
}


/* TEST FUNCTIONS */
void testContiguousInsert(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads*threadCapacity);
    const double computeTime = parallelInsert(threadCapacity, numThreads, keyVector);
    printf("Contiguous insert for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computetime));
    deleteTree();
}

void testRandomInsert(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelInsert(threadCapacity, numThreads, keyVector);
    printf("Random insert for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computeTime));
    deleteTree();
}

void testContiguousDeleteContiguousTree(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    printf("Contiguous delete on contiguous tree for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computeTime));
    deleteTree();
}

void testContiguousDeleteRandomTree(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    keyVector = getBlockVector(0, numThreads * threadCapacity);
    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    printf("Contiguous delete on random tree for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computeTime));
    deleteTree();
}

void testRandomDeleteContiguousTree(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    printf("Random delete on contiguous tree for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computeTime));
    deleteTree();
}

void testRandomDeleteRandomTree(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelDelete(threadCapacity, numThreads, keyVector);
    printf("Random delete on random tree for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computeTime));
    deleteTree();
}

void testContiguousSearch(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    const double computeTime = parallelSearch(threadCapacity, numThreads, keyVector);
    printf("Random search for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computeTime));
    deleteTree();
}

void testRandomSearch(int numThreads, int threadCapacity) {
    initTree();
    std::vector<int> keyVector = getBlockVector(0, numThreads * threadCapacity);
    parallelInsert(threadCapacity, numThreads, keyVector);

    keyVector = getShuffledVector(0, numThreads * threadCapacity);
    const double computeTime = parallelSearch(threadCapacity, numThreads, keyVector);
    printf("Random search for %d capacity and %d threads: %ld milliseconds, %ld operations per millisecond\n", threadCapacity, numThreads, computeTime, threadCapacity * numThreads / max((int64_t)1, computeTime));
    deleteTree();
}

/* MAIN FUNCTION */
int main(int argc, char const *argv[]) {
    std::vector<int> numThreads = {1, 4, 16, 64, 128};
    std::vector<int> threadCapacities = {100000, 10000};
    std::vector<int> impl = {1, 2, 3};
    for (int m : impl) {
        IMPL = m;
        printImpl();
        for (int capacity : threadCapacities) {
            for (int threads : numThreads) {
                testContiguousInsert(threads, capacity/threads);
                testRandomInsert(threads, capacity/threads);
                testContiguousDeleteContiguousTree(threads, capacity/threads);
                testContiguousDeleteRandomTree(threads, capacity/threads);
                testRandomDeleteContiguousTree(threads, capacity/threads);
                testRandomDeleteRandomTree(threads, capacity/threads);
                testContiguousSearch(threads, capacity/threads);
                testRandomSearch(threads, capacity/threads);
                testRandomDelete(threads, capacity/threads);
                testRandomFind(threads, capacity/threads);
            }
        }
    }
    return 0;
}