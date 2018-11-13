#include <string.h>
#include <assert.h>
#include "patterns.h"
#include "cilk/cilk.h"
#include "cilk/cilk_api.h"

#include <stdio.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#define min(x, y) ((x < y) ? x : y)
#define max(x, y) ((x > y) ? x : y)
#pragma GCC diagnostic pop

void map (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2)) {
    assert (dest != NULL);
    assert (src != NULL);
    assert (worker != NULL);

    // Ignore the warning
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

    // Define the grainsize
	//#pragma cilk grainsize = max(1024, min(nJob/(__cilkrts_get_nworkers()), 2048))
	#pragma cilk grainsize = __cilkrts_get_nworkers()
    cilk_for (int i = 0; i < nJob; i++) {
        worker(dest + i * sizeJob, src + i * sizeJob);
    }
    #pragma GCC diagnostic pop

    // Alternative implementation when hardware vectorization is possible
	/*#pragma simd
    for (int i=0; i < nJob; i++) {
    	worker(dest + i * sizeJob, src + i * sizeJob);
    }*/
}

void map_seq (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2)) {
    assert (dest != NULL);
    assert (src != NULL);
    assert (worker != NULL);
    for (int i=0; i < nJob; i++) {
        worker(dest + i * sizeJob, src + i * sizeJob);
    }
}

void reduce (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
    /* To be implemented */
    assert (dest != NULL);
    assert (src != NULL);
    assert (worker != NULL);
    if (nJob > 1) {
        memcpy (dest, src, sizeJob);
        for (int i = 1;  i < nJob;  i++)
            worker(dest, dest, src + i * sizeJob);
    }
}

void scan (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
    /* To be implemented */
    assert (dest != NULL);
    assert (src != NULL);
    assert (worker != NULL);
    if (nJob > 1) {
        memcpy (dest, src, sizeJob);
        for (int i = 1;  i < nJob;  i++)
            worker(dest + i * sizeJob, src + i * sizeJob, dest + (i-1) * sizeJob);
    }
}

int pack (void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
    /* To be implemented */
    int pos = 0;
    for (int i=0; i < nJob; i++) {
        if (filter[i]) {
            memcpy (dest + pos * sizeJob, src + i * sizeJob, sizeJob);
            pos++;
        }
    }
    return pos;
}

void gather (void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter, int nFilter) {
    /* To be implemented */
    for (int i=0; i < nFilter; i++) {
        memcpy (dest + i * sizeJob, src + filter[i] * sizeJob, sizeJob);
    }
}

void scatter (void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
    /* To be implemented */
    for (int i=0; i < nJob; i++) {
        memcpy (dest + filter[i] * sizeJob, src + i * sizeJob, sizeJob);
    }
}

void pipeline (void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {

	unsigned int avg_batch_size = nJob / nWorkers;
	unsigned int iterations = nWorkers + (nWorkers-1);

	int last_worker, max_worker, batch_start, current_batch_size; // Choose better names

	memcpy(dest, src, nJob*sizeJob);

	for(int i = 0; i < iterations; i++) {
		max_worker = min(i, nWorkers-1);
		last_worker = i - max_worker;

		// Compute each worker on the respective batch
		for(int j = max_worker; j >= last_worker; j--) {
			batch_start = (i-j)*avg_batch_size;

			current_batch_size = (nJob - (batch_start + 2*avg_batch_size) < 0) ? nJob - batch_start : avg_batch_size;

			// Compute the worker on the current batch
			/*cilk_for(int k = 0; k < current_batch_size; k++) {
    			workerList[j](dest + batch_start*sizeJob + k * sizeJob, dest + batch_start*sizeJob + k * sizeJob);
    		}*/
			map(dest + batch_start*sizeJob, dest + batch_start*sizeJob, current_batch_size, sizeJob, workerList[j]);
		}
	}
}

void pipeline_seq (void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
    for (int i=0; i < nJob; i++) {
        memcpy (dest + i * sizeJob, src + i * sizeJob, sizeJob);
        for (int j = 0;  j < nWorkers;  j++)
            workerList[j](dest + i * sizeJob, dest + i * sizeJob);
    }
}

void farm (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers) {
    /* To be implemented */
    map (dest, src, nJob, sizeJob, worker);
}
