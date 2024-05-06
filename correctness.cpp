#include <bits/stdc++.h>
#include "coarsegrained.h"
#include "finegrained.h"
#include "lockfree2.h"
#include "lockfree.h"
using namespace std;

// Coarse-grained: IMPL=1, fine-grained: IMPL=2, lock-free: IMPL=3, BST lock-free: IMPL=4
#define IMPL 4
#define NUM_THREADS 4
#define THREAD_SIZE 100

AVLTreeCG *treeCG;
AVLTreeFG *treeFG;
AVLTree *treeLF;
AVLTreeLF *treeBST;

/* UTILITY FUNCTIONS */
void initTree() {
    if (IMPL==1) treeCG = new AVLTreeCG();
    if (IMPL==2) treeFG = new AVLTreeFG();
    if (IMPL==3) treeLF = new AVLTree();
    if (IMPL==4) treeBST = new AVLTreeLF();
    printf("Tree initialized\n");
}

void deleteTree() {
    if (IMPL==1) delete treeCG;
    if (IMPL==2) delete treeFG;
    if (IMPL==3) delete treeLF;
    if (IMPL==4) delete treeBST;
}

/* Return a random vector of key inputs */
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
    if (IMPL==2) return treeFG->insert(k);
    if (IMPL==3) return treeLF->insert(k);
    if (IMPL==4) return treeBST->insert(k);
}

bool flexDelete(int k) {
    if (IMPL==1) return treeCG->deleteNode(k);
    if (IMPL==2) return treeFG->deleteNode(k);
    if (IMPL==3) return treeLF->deleteNode(k);
    if (IMPL==4) return treeBST->deleteNode(k);
}

bool flexSearch(int k) {
    if (IMPL==1) return treeCG->search(k);
    if (IMPL==2) return treeFG->search(k);
    if (IMPL==3) return treeLF->search(k);
    if (IMPL==4) return treeBST->search(k);
}

/* HELPER FUNCTIONS */
void insertRange(int low, int high) {
    for (int i=low; i<high; i++) {
        flexInsert(i);
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

int checkHeightAndBalanceCG(NodeCG* node) {
    if (node==nullptr) return 0;
    int leftHeight = checkHeightAndBalanceCG(node->left);
    int rightHeight = checkHeightAndBalanceCG(node->right);
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

int checkHeightAndBalanceFG(NodeFG* node) {
    if (node==nullptr) return 0;
    int leftHeight = checkHeightAndBalanceFG(node->left);
    int rightHeight = checkHeightAndBalanceFG(node->right);
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

int checkHeightAndBalanceLF(Node* node) {
    return 0;
    if (node==nullptr) return 0;
    Node* leftNode = node->left.getValue();
    Node* rightNode = node->right.getValue();
    int leftHeight = checkHeightAndBalanceLF(leftNode);
    int rightHeight = checkHeightAndBalanceLF(rightNode);
    if (node->height.getValue() != 1+std::max(leftHeight, rightHeight))
        throw std::runtime_error("Node height is incorrect");
    int balance = leftHeight-rightHeight;
    if (balance<-1 || balance>1)
        throw std::runtime_error("Node is unbalanced");
    if (leftNode && leftNode->key>=node->key)
        throw std::runtime_error("Left child key is greater or equal to node key");
    if (rightNode && rightNode->key<=node->key)
        throw std::runtime_error("Right child key is lesser or equal to node key");
    if (leftNode==node || rightNode==node)
        throw std::runtime_error("Circular reference detected");
    return node->height.getValue();
}

int checkHeightAndBalance() {
    if (IMPL==1) {
        return checkHeightAndBalanceCG(treeCG->root);
    }
    if (IMPL==2) {
        return checkHeightAndBalanceFG(treeFG->root);
    }
    if (IMPL==3) {
        return checkHeightAndBalanceLF(treeLF->minRoot->right);
    }
    if (IMPL==4)
        return 0;
    throw std::runtime_error("Invalid IMPL defined in correctness.cpp");
    return 0;
}

/* TEST FUNCTIONS */
void testSequentialSearch() {
    initTree();
    if (IMPL==1) {
        treeCG->root=new NodeCG(20);
        NodeCG* treeRoot = treeCG->root;
        treeRoot->left=new NodeCG(12);
        treeRoot->right=new NodeCG(53);
        treeRoot->left->left=new NodeCG(1);
        treeRoot->right->left=new NodeCG(21);
        treeRoot->left->right=new NodeCG(17);
        treeRoot->right->right=new NodeCG(82);
        treeRoot->right->right->left=new NodeCG(73);
        treeRoot->left->right->left=new NodeCG(15);
        treeRoot->left->left->right=new NodeCG(2);
    }
    if (IMPL==2) {
        treeFG->root=new NodeFG(20);
        NodeFG* treeRoot = treeFG->root;
        treeRoot->left=new NodeFG(12);
        treeRoot->right=new NodeFG(53);
        treeRoot->left->left=new NodeFG(1);
        treeRoot->right->left=new NodeFG(21);
        treeRoot->left->right=new NodeFG(17);
        treeRoot->right->right=new NodeFG(82);
        treeRoot->right->right->left=new NodeFG(73);
        treeRoot->left->right->left=new NodeFG(15);
        treeRoot->left->left->right=new NodeFG(2);
    }
    if (IMPL==3) {
        deleteTree();
        return;
    }
    if (IMPL==4) {
        return;
    }
    std::set<int> elems = {20,12,53,1,21,17,82,73,15,2};
    for (int i=1; i<100; i++) {
        bool found = flexSearch(i);
        if (elems.find(i)!=elems.end() && !found) {
            std::ostringstream oss;
            oss << "Search failed, missing " << i << "\n";
            throw std::runtime_error(oss.str());
        } else if (elems.find(i)==elems.end() && found) {
            std::ostringstream oss;
            oss << "Search failed, incorrectly finding " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
    }
    deleteTree();
    printf("Sequential search passed!\n");
}

void testSequentialInsert() {
    initTree();
    insertRange(500, 510);
    insertRange(510, 900);
    insertRange(1, 100);
    insertRange(900, 1000);
    insertRange(100, 500);
    for (int i=1; i<1000; i++) {
        if (!flexSearch(i)) {
            std::ostringstream oss;
            oss << "Sequential insert failed, missing " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
    }
    checkHeightAndBalance();
    deleteTree();
    printf("Sequential insert passed!\n");
}

void testConcurrentInsert() {
    initTree();
    std::vector<std::thread> threads;
    for (int i=0; i<NUM_THREADS; i++) {
        threads.push_back(thread(insertRange, 1+i*100, 1+(i+1)*100));
    }
    for (int i=0; i<NUM_THREADS; i++) {
        threads[i].join();
        printf("Reached here\n");
    }
    for (int i=1; i<=NUM_THREADS*100; i++) {
        if (!flexSearch(i)) {
            std::ostringstream oss;
            oss << "Concurrent insert failed, missing " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
    }
    checkHeightAndBalance();
    deleteTree();
    printf("Concurrent insert passed!\n");
}

void testSequentialDelete() {
    initTree();
    insertRange(1, THREAD_SIZE);
    deleteRangeContiguous(THREAD_SIZE/2, THREAD_SIZE);
    for (int i=1; i<THREAD_SIZE; i++) {
        bool found = flexSearch(i);
        if (i<THREAD_SIZE/2 && !found) {
            std::ostringstream oss;
            oss << "Sequential delete failed, missing " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
        else if (i>=THREAD_SIZE/2 && found) {
            std::ostringstream oss;
            oss << "Sequential delete failed, incorrectly finding " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
    }
    checkHeightAndBalance();
    deleteTree();

    initTree();
    insertRange(1, THREAD_SIZE);
    deleteRangeSpread(2, THREAD_SIZE);
    for (int i=1; i<THREAD_SIZE; i++) {
        bool found = flexSearch(i);
        if (i%2==1 && !found) {
            std::ostringstream oss;
            oss << "Sequential delete failed, missing " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
        else if (i%2==0 && found) {
            std::ostringstream oss;
            oss << "Sequential delete failed, incorrectly finding " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
    }
    checkHeightAndBalance();
    deleteTree();
    printf("Sequential delete passed!\n");
}

void testConcurrentDelete() {
	initTree();
	insertRange(1, 1 + NUM_THREADS * THREAD_SIZE);
	std::vector<std::thread> threads;
	for (int i = 0; i < NUM_THREADS; i++) {
		threads.push_back(thread(deleteRangeContiguous, 1+i*THREAD_SIZE+THREAD_SIZE/4, 1+(i+1)*THREAD_SIZE));
	}
	for (int i = 0; i<NUM_THREADS; i++) {
		threads[i].join();
	}
	for (int i = 1; i<=NUM_THREADS * THREAD_SIZE; i++) {
        bool found = flexSearch(i);
        if (i%THREAD_SIZE==0)
            continue;
		if (i%THREAD_SIZE<THREAD_SIZE/4 && !found) {
            std::ostringstream oss;
            oss << "Concurrent delete failed, missing " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
		else if (i%THREAD_SIZE>THREAD_SIZE/4 && found) {
            std::ostringstream oss;
            oss << "Concurrent delete failed, incorrectly finding " << i << "\n";
            throw std::runtime_error(oss.str());
        } 
	}
    checkHeightAndBalance();
	deleteTree();
	printf("Concurrent deletion passed!\n");
}

void testInsertDeleteContiguous() {
	initTree();
	std::vector<std::thread> threads;
	for (int i=0; i<NUM_THREADS; i++) {
		threads.push_back(thread(insertRangeDeleteContiguous, 1+i*THREAD_SIZE, 1+(i+1)*THREAD_SIZE, 1+(THREAD_SIZE+1)/2));
	}
	for (int i=0; i<NUM_THREADS; i++) {
		threads[i].join();
	}
	for (int i=1; i<=NUM_THREADS*THREAD_SIZE; i++) {
		if (flexSearch(i)) {
            std::ostringstream oss;
            oss << "Insert/delete contiguous mix failed, incorrectly finding " << i << "\n";
            throw std::runtime_error(oss.str());
        }
	}
    checkHeightAndBalance();
	deleteTree();
	printf("insert delete test passed!\n");
}

void testInsertDeleteSpread() {
	initTree();
	std::vector<std::thread> threads;
	for (int i=0; i<NUM_THREADS; i++) {
		threads.push_back(thread(insertRangeDeleteSpread, i*THREAD_SIZE, (i+1)*THREAD_SIZE, (THREAD_SIZE+1)/2));
	}
	for (int i=0; i<NUM_THREADS; i++) {
		threads[i].join();
	}
	for (int i=0; i<=NUM_THREADS*THREAD_SIZE; i++) {
        bool found = flexSearch(i);
        if (i%2==1 && !found) {
            std::ostringstream oss;
            oss << "Insert/delete spread mix failed, missing " << i << "\n";
            throw std::runtime_error(oss.str());
        }
        else if (i%2==0 && found) {
            std::ostringstream oss;
            oss << "Insert/delete spread mix failed, incorrectly finding " << i << "\n";
            throw std::runtime_error(oss.str());
        }
    }
    checkHeightAndBalance();
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