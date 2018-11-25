#include <stdlib.h>
#include <string.h>
#include <cilk/cilk.h>
#include <assert.h>
#include "prefix_scan.h"

/*
 * Implementation of parallel Prefix-Scan algorithm using an array-based binary tree.
 */

/* Implementation of Binary Tree Node
 * 
 * A tree node is a contiguous block of memory, as if a struct:
 * Node {
 *	 size_t range[2]
 *	 size_job value
 *	 size_job from_left
 * 	}
 *
 * The number of elements of the Binary tree is known at the start, so an array-based binary tree 
 * implementation is used. The array based implementation allows for a single big (contiguous, cache friendly) 
 * memory allocation instead of several small, on-demand (disperse/fragmented, cache unfriendly) 
 * allocations. This provides better results, since less time is spent on mallocs (system calls), which are
 * inherently slow.
 */	
 
 // Size of range[] in a tree node
static size_t RANGE_MEM_SIZE = 2 * sizeof(size_t);

// Total size of a tree node
static size_t tree_node_size = 0;

/*
 * Get the total number of elements of a binary tree, given the size of the last tree level.
 */
static size_t get_total_tree_size(size_t max_width) {
	
	return 2 * max_width - 1;
}

/*
 * Up pass of the prefix scan algorithm, which builds a binary tree given an input.
 */
static void up_pass(void *tree, size_t node_index, void *input, size_t low, size_t high, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3)) {
	
	size_t range[2] = {low, high};
	memcpy(tree, &range, 2 * sizeof(size_t));
	
	if(low + 1 == high) {
		memcpy(tree + RANGE_MEM_SIZE, input + low * size_job, size_job);
	} else {
		size_t mid = (low + high) / 2;
		
		void *left_child = tree + (2 *  node_index + 1) * tree_node_size;
		void *right_child = left_child + tree_node_size;
		
		up_pass(left_child, node_index + 1, input, low, mid, size_job, worker);
		cilk_spawn up_pass(right_child, node_index + 2, input, mid, high, size_job, worker);
		
		cilk_sync;
		worker(tree + RANGE_MEM_SIZE , left_child + RANGE_MEM_SIZE, right_child + RANGE_MEM_SIZE );
	}
	
}

/*
 * Down pass of the prefix scan algorithm, which fills a binary tree's from_left field, outputting the results of the leaves to an output.
 */
static void down_pass(void *tree, size_t node_index, void *output, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3)) {
	
	size_t *range = ((size_t *) tree);
	
	if(*range + 1 == *(range +1)) {
		worker(output + *range * size_job, tree + RANGE_MEM_SIZE, tree + RANGE_MEM_SIZE + size_job);
	} else {
		void *left_child = tree + (2 * node_index + 1) * tree_node_size;
		void *right_child = left_child + tree_node_size;
		
		memcpy(left_child + RANGE_MEM_SIZE + size_job, tree + RANGE_MEM_SIZE + size_job, size_job);
		worker(right_child + RANGE_MEM_SIZE + size_job, tree + RANGE_MEM_SIZE + size_job, left_child + RANGE_MEM_SIZE);
		
		down_pass(left_child, node_index + 1, output, size_job, worker);
		cilk_spawn down_pass(right_child, node_index + 2, output, size_job, worker);
		
		cilk_sync;
	}	
}

/*
 * Execute prefix scan algorithm.
 */
void prefix_scan(void *input, void *output, size_t n_jobs, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3), void *neutral_element) {
	
	tree_node_size = RANGE_MEM_SIZE + 2 * size_job;
	
	void *tree = malloc(tree_node_size *  get_total_tree_size(n_jobs));
	
	assert(tree != NULL);
	
	up_pass(tree, 0, input, 0, n_jobs, size_job, worker);
	
	memcpy(tree + RANGE_MEM_SIZE + size_job, neutral_element, size_job);
	
	down_pass(tree, 0, output, size_job, worker);
	
	free(tree);
}