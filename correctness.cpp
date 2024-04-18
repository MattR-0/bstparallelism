#include "coarsegrained.h"

using namespace std;

// Coarse-grained: IMPL=1, fine-grained: IMPL=2, lock-free: IMPL=3
#define IMPL 1
#define NUM_THREADS 8
#define THREAD_SIZE 1000

AVLTreeCG *treeCG;

/* UTILITY FUNCTIONS */
void initTree() {
    if (IMPL==1) treeCG = new AVLTreeCG();
}

void deleteTree() {
    if (IMPL==1) delete treeCG;
}

Node *getRoot() {
    if (IMPL==1) return treeCG->root;
}

std::vector<int> getShuffledVector(int low, int high) {
    std::vector<int> v(high-low);
    std::iota(v.begin(), v.end(), low);
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(v.begin(), v.end(), g);
    return v;
}

bool flexInsert(int k) {
    if (IMPL==1) return treeCG->insert(k);
}

bool flexDelete(int k) {
    if (IMPL==1) return treeCG->deleteNode(k);
}

bool flexSearch(int k) {
    if (IMPL==1) return treeCG->search(k);
}

/* HELPER FUNCTIONS */
void insertRange(int low, int high, std::std::vector<int> keyVector) {
    for (int i=low; i<high; i++) {
        flexInsert(keyVector[i]);
    }
}

void deleteRangeContiguous(int low, int high) {
	for (int i=low; i<high; i++) {
		flexDelete(i);
	}
}

void deleteRangeSpread(int low, int high) {
    for (int i=low; i<high; i+=2) {
        flexDelete(i);
    }
}

void insertRangeDeleteContiguous(int low, int high, int interval) {
    for (int i=low; i<high; i+=interval) {
        insertRange(i, min(high, i+interval));
        deleteRangeContiguous(i, min(high, i+interval));
    }
}

void insertRangeDeleteSpread(int low, int high, int interval) {
    for (int i=low; i<high; i+=interval) {
        insertRange(i, min(high, i+interval));
        deleteRangeSpread(i, min(high, i+interval));
    }
}

int checkHeightAndBalance(Node* node) {
    if (node==nullptr) return 0;
    int leftHeight = checkHeightAndBalance(node->left);
    int rightHeight = checkHeightAndBalance(node->right);
    if (node->height != 1+std::max(leftHeight, rightHeight))
        throw std::runtime_error("Node height is incorrect");
    int balance = leftHeight-rightHeight;
    if (balance<-1 || balance>1)
        throw std::runtime_error("Node is unbalanced");
    if (node->left && node->left->key>=node->key)
        throw std::runtime_error("Left child key is greater or equal to node key");
    if (node->right && node->right->key<=node->key)
        throw std::runtime_error("Right child key is lesser or equal to node key");
    if (node->left==node || node->right==node)
        throw std::runtime_error("Circular reference detected");
    return node->height;
}

/* TEST FUNCTIONS */
void testSequentialSearch() {
    initTree();
    if (IMPL==1) treeCG->root=Node(20);
    Node* treeRoot = getRoot();
    treeRoot->left=Node(12);
    treeRoot->right=Node(53);
    treeRoot->left->left=Node(0);
    treeRoot->right->left=Node(21);
    treeRoot->left->right=Node(17);
    treeRoot->right->right=Node(82);
    treeRoot->right->right->left=Node(73);
    treeRoot->left->right->left=Node(15);
    treeRoot->left->left->right=Node(2);
    std::set<int> elems = {20,12,53,0,21,17,82,73,15,2};
    for (int i=0; i<100; i++) {
        bool found = flexSearch(i);
        if (elems.contains(i) && !found)
            throw std::runtime_error("Search failed, missing %d\n", i);
        else if (!elems.contains(i) && found)
            throw std::runtime_error("Search failed, incorrectly finding %d\n", i);
    }
    printf("Sequential search passed!\n");
}

void testSequentialInsert() {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, 1000);
    insertRange(0, 1000, keyVector);
    for (int i=0; i<1000; i++) {
        if (!flexSearch(i))
            throw std::runtime_error("Sequential insert failed, missing %d\n", i);
    }
    checkHeightAndBalance(getRoot());
    deleteTree();
    printf("Sequential insert passed!\n");
}

void testConcurrentInsert() {
    initTree();
    std::vector<int> keyVector = getShuffledVector(0, NUM_THREADS*1000);
    std::std::vector<thread> threads;
    for (int i=0; i<NUM_THREADS; i++) {
        threads.push_back(thread(insertRange, i*100, (i+1)*100));
    }
    for (int i=0; i<NUM_THREADS; i++) {
        threads[i].join();
    }
    for (int i=0; i<NUM_THREADS*1000; i++) {
        if (!flexSearch(i))
            throw std::runtime_error("Concurrent insert failed, missing %d\n", i);
    }
    checkHeightAndBalance(getRoot());
    deleteTree();
    printf("Concurrent insert passed!\n");
}

void testSequentialDelete() {
    initTree();
    insertRangeRandom(0, THREAD_SIZE);
    deleteRangeContiguous(THREAD_SIZE/2, THREAD_SIZE);
    for (int i=0; i<THREAD_SIZE; i++) {
        bool found = flexSearch(i)
        if (i<THREAD_SIZE/2 && !found)
            throw std::runtime_error("Sequential delete failed, missing %d\n", i);
        else if (i>=THREAD_SIZE/2 && found)
            throw std::runtime_error("Sequential delete failed, incorrectly finding %d\n", i);
    }
    checkHeightAndBalance(getRoot());
    deleteTree();

    initTree();
    insertRangeRandom(0, THREAD_SIZE);
    deleteRangeSpread(0, THREAD_SIZE);
    for (int i=0; i<THREAD_SIZE; i++) {
        bool found = flexSearch(i);
        if (i%2==1 && !found)
            throw std::runtime_error("Sequential delete failed, missing %d\n", i);
        else if (i%2==0 && found)
            throw std::runtime_error("Sequential delete failed, incorrectly finding %d\n", i);
    }
    checkHeightAndBalance(getRoot());
    deleteTree();
    printf("Sequential delete passed!\n");
}

void testConcurrentDelete() {
	initTree();
	insertRangeRandom(0, NUM_THREADS * THREAD_SIZE);
	std::vector<thread> threads;
	for (int i = 0; i < NUM_THREADS; i++) {
		threads.push_back(thread(deleteRange, i*THREAD_SIZE+THREAD_SIZE/4, (i+1)*THREAD_SIZE));
	}
	for (int i = 0; i<NUM_THREADS; i++) {
		threads[i].join();
	}
	for (int i = 0; i<NUM_THREADS * THREAD_SIZE; i++) {
        bool found = flexSearch(i);
		if (i%THREAD_SIZE<THREAD_SIZE/4 && !found)
            throw std::runtime_error("Concurrent delete failed, missing %d\n", i);
		else if (i%THREAD_SIZE>=THREAD_SIZE/4 && found)
            throw std::runtime_error("Concurrent delete failed, incorrectly finding %d\n", i);
	}
    checkHeightAndBalance(getRoot());
	deleteTree();
	printf("Concurrent deletion passed!\n");
}

void testInsertDeleteContiguous() {
	initTree();
	std::vector<thread> threads;
	for (int i=0; i<NUM_THREADS; i++) {
		threads.push_back(thread(insertRangeDeleteContiguous, i*THREAD_SIZE, (i+1)*THREAD_SIZE));
	}
	for (int i=0; i<NUM_THREADS; i++) {
		threads[i].join();
	}
	for (int i=0; i<NUM_THREADS*THREAD_SIZE; i++) {
		if (flexSearch(i))
            throw std::runtime_error("Insert/delete contiguous mix failed, incorrectly finding %d\n", i);
	}
    checkHeightAndBalance(getRoot());
	deleteTree();
	printf("insert delete test passed!\n");
}

void testInsertDeleteSpread() {
	initTree();
	std::vector<thread> threads;
	for (int i=0; i<NUM_THREADS; i++) {
		threads.push_back(thread(insertRangeDeleteSpread, i*THREAD_SIZE, (i+1)*THREAD_SIZE));
	}
	for (int i=0; i<NUM_THREADS; i++) {
		threads[i].join();
	}
	for (int i=0; i<NUM_THREADS*THREAD_SIZE; i++) {
        bool found = flexSearch(i);
        if (i%2==1 && !found)
            throw std::runtime_error("Insert/delete spread mix failed, missing %d\n", i);
        else if (i%2==0 && found)
            throw std::runtime_error("Insert/delete spread mix failed, incorrectly finding %d\n", i);
    }
    checkHeightAndBalance(getRoot());
	deleteTree();
	printf("insert delete test passed!\n");
}

/* MAIN FUNCTION */
int main() {
    testSequentialSearch();
	testSequentialInsert();
	testSequentialDelete();
	for (int i=0; i<10; i++) {
		testConcurrentInsert();
	}
	for (int i=0; i<10; i++) {
		testConcurrentDelete();
	}
	for (int i=0; i<10; i++) {
		testInsertDeleteContiguous();
	}
	for (int i=0; i<10; i++) {
		testInsertDeleteSpread();
	}
}
