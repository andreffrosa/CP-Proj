#include <string.h>
#include <assert.h>
#include "patterns.h"
#include "cilk/cilk.h"
#include "cilk/cilk_api.h"

#include <stdio.h>

void map (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2)) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (worker != NULL);

	// Define the grainsize
	//#pragma cilk grainsize = max(1024, min(nJob/(__cilkrts_get_nworkers()), 2048))
	cilk_for (int i = 0; i < nJob; i++)
	worker(dest + i * sizeJob, src + i * sizeJob);
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
	assert (dest != NULL);
	assert (src != NULL);
	assert (workerList != NULL);

	memcpy(dest, src, nJob*sizeJob);

	// Start of the pipeline
	unsigned int limit = nWorkers-1;
	for(int i = 0; i < limit; i++) {
		// Compute each worker
		for( int j = 0; j <= i; j++) {
			void* job = dest + (i-j)*sizeJob;
			cilk_spawn workerList[j](job, job);
		}
		cilk_sync;
	}

	// Normal functioning of the pipeline
	limit = nJob;
	for(int i =  nWorkers-1; i < limit; i++) {
		// Compute each worker
		for( int j = 0; j < nWorkers; j++) {
			void* job = dest + (i-j)*sizeJob; // inicializar o i logo a nWorkers-1 ?
			cilk_spawn workerList[j](job, job);
		}
		cilk_sync;
	}

	// Finish of the ramaining tasks
	limit = nJob + nWorkers-1;
	for(int i = nJob; i < limit; i++) {
		// Compute each worker
		for( int j = i - nJob + 1; j < nWorkers; j++) {
			void* job = dest + (i-j)*sizeJob;
			cilk_spawn workerList[j](job, job);
		}
		cilk_sync;
	}
}

void pipeline_farm (void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers, size_t nFarms) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (workerList != NULL);
	assert (nFarms > 0);

	if( nFarms == 1)
		pipeline(dest, src, nJob, sizeJob, workerList, nWorkers);
	else {
		cilk_for(size_t i = 0; i < nJob; i++) {
			memcpy(dest + i*sizeJob, src + i*sizeJob, sizeJob);
		}

		size_t nBatches = (nJob / nFarms) + ( nJob % nFarms == 0 ? 0 : 1);

		// Start of the pipeline
		unsigned int limit = nWorkers-1;
		for(int i = 0; i < limit; i++) {
			// Compute each worker
			for( int j = 0; j <= i; j++) {
				size_t length = (i-j == nBatches-1) ? nJob-(nBatches-1)*nFarms : nFarms;
				//cilk_spawn workerList[j](job, job);
				cilk_for( int k = 0; k < length; k++) {
					void* job = dest + ((i-j)*batchSize+k)*sizeJob;
					workerList[j](job, job);
				}
			}
			//cilk_sync;
		}

		// Normal functioning of the pipeline
		limit = nBatches;
		for(int i =  nWorkers-1; i < limit; i++) {
			// Compute each worker
			for( int j = 0; j < nWorkers; j++) {
				size_t length = (i-j == nBatches-1) ? nJob-(nBatches-1)*nFarms : nFarms;
				//void* job = dest + (i-j)*sizeJob; // inicializar o i logo a nWorkers-1 ?
				//cilk_spawn workerList[j](job, job);
				cilk_for( int k = 0; k < length; k++) {
					void* job = dest + ((i-j)*batchSize+k)*sizeJob;
					workerList[j](job, job);
				}
			}
			//cilk_sync;
		}

		// Finish of the ramaining tasks
		limit = nBatches + nWorkers-1;
		for(int i = nBatches; i < limit; i++) {
			// Compute each worker
			for( int j = i - nBatches + 1; j < nWorkers; j++) {
				size_t length = (i-j == nBatches-1) ? nJob-(nBatches-1)*nFarms : nFarms;
				//void* job = dest + (i-j)*sizeJob;
				//cilk_spawn workerList[j](job, job);
				cilk_for( int k = 0; k < length; k++) {
					void* job = dest + ((i-j)*batchSize+k)*sizeJob;
					workerList[j](job, job);
				}
			}
			//cilk_sync;
		}
	}
}


/*void multiple_pipeline (void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {

	//unsigned int avg_batch_size = nJob / nWorkers;
	// Ignore the warning
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

	unsigned int avg_batch_size = __cilkrts_get_nworkers();
	unsigned int iterations = nJob + nWorkers-1;
	//unsigned int iterations = nWorkers + (nWorkers-1);

	int last_worker, max_worker, batch_start, current_batch_size; // Choose better names

	memcpy(dest, src, nJob*sizeJob);

	printf("[%f", *(double*)(src));
		for(int s=1; s < nJob; s++) {
			printf(", %f", *((double*)(src)+s));
		}
		printf("]\n");

	printf("%d <= i < %d\n", 0, iterations);
	for(int i = 0; i < iterations; i++) {
		max_worker = min(i, nWorkers-1);
		last_worker = i - max_worker;

		printf("%d <= j <= %d\n", max_worker, last_worker);
		// Compute each worker on the respective batch
		for(int j = max_worker; j >= last_worker; j--) {
			batch_start = (i-j)*avg_batch_size;

			current_batch_size = (nJob - (batch_start + 2*avg_batch_size) < 0) ? nJob - batch_start : avg_batch_size;

			// Compute the worker on the current batch
 *//*cilk_for(int k = 0; k < current_batch_size; k++) {
    			workerList[j](dest + batch_start*sizeJob + k * sizeJob, dest + batch_start*sizeJob + k * sizeJob);
    		}*//*
			printf("%d <= batch <= %d\n", batch_start, batch_start+current_batch_size-1);
			cilk_spawn map(dest + batch_start*sizeJob, dest + batch_start*sizeJob, current_batch_size, sizeJob, workerList[j]);

			//cilk_spawn workerList[j](dest + batch_start*sizeJob, dest + batch_start*sizeJob);
			//printf("%d\n", current_batch_size);
		}
		cilk_sync;
		printf("[%f", *(double*)(dest));
		for(int s=1; s < nJob; s++) {
			printf(", %f", *((double*)(dest)+s));
		}
		printf("]\n");
	}

	printf("seq\n");
	pipeline_seq(dest, src, nJob, sizeJob, workerList, nWorkers);
	printf("[%f", *(double*)(dest));
		for(int s=1; s < nJob; s++) {
			printf(", %f", *((double*)(dest)+s));
		}
		printf("]\n");

    	#pragma GCC diagnostic pop
}*/

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
