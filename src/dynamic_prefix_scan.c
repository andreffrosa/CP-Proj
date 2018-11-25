#include <stdlib.h>
#include <string.h>
#include <cilk/cilk.h>
#include <assert.h>
#include "prefix_scan.h"

/*
 * Implementation of parallel Prefix-Scan algorithm using a dynamic binary tree.
 * This implementation is much slower than a static implementation, since instead of a single 
 * large memory allocation at the start, there is a malloc (system call) each time a tree node is created,
 * which incurs a significant amount of overhead.
 */

/*
 * Binary_Node struct, that represents a single node that makes up a binary tree.
 */
typedef struct Binary_Node {
	size_t range_start;
	void *value;
	void *from_left;
	struct Binary_Node *left_child;
	struct Binary_Node *right_child;
} Binary_Node;

/*
 * Up pass of the prefix scan algorithm, which builds a binary tree given an input.
 */
static void up_pass(Binary_Node *tree, void *input, size_t low, size_t high, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3)) {
	
	tree->range_start = low;
	tree->value = malloc(size_job);
	tree->from_left = malloc(size_job);
	
	if(low + 1 == high) {
		tree->left_child = NULL;
		tree->right_child = NULL;
		memcpy(tree->value, input + low * size_job, size_job);
	} else {
		size_t mid = (low + high) / 2;
		
		tree->left_child = malloc(sizeof(Binary_Node));
		tree->right_child = malloc(sizeof(Binary_Node));
		
		up_pass(tree->left_child, input, low, mid, size_job, worker);
		cilk_spawn up_pass(tree->right_child, input, mid, high, size_job, worker);
		
		cilk_sync;
		worker(tree->value, tree->left_child->value, tree->right_child->value);
	}
}

/*
 * Down pass of the prefix scan algorithm, which fills a binary tree's from_left field, outputting the results of the leaves to an output.
 */
static void down_pass(Binary_Node *tree, void *output, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3)) {
	
	if(tree->left_child == NULL && tree->right_child == NULL) {
		worker(output + tree->range_start * size_job, tree->from_left, tree->value);
	} else {
		memcpy(tree->left_child->from_left, tree->from_left, size_job);
		worker(tree->right_child->from_left, tree->from_left, tree->left_child->value);
		
		down_pass(tree->left_child, output, size_job, worker);
		cilk_spawn down_pass(tree->right_child, output, size_job, worker);
	}	
	
	cilk_sync;
	
	free(tree->value);
	free(tree->from_left);
	free(tree);
}

/*
 * Execute prefix scan algorithm.
 */
void prefix_scan(void *input, void *output, size_t n_jobs, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3)) {
		
	Binary_Node *tree = malloc(sizeof(Binary_Node));
	
	assert(tree != NULL);
	
	up_pass(tree, input, 0, n_jobs, size_job, worker);
	
	void *neutral_element = malloc(size_job);
	worker(neutral_element, NULL, NULL);
	memcpy(tree->from_left, neutral_element, size_job);
	free(neutral_element);
	
	down_pass(tree, output, size_job, worker);
}