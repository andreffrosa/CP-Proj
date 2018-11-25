#include <string.h>
#include <assert.h>
#include "patterns.h"
#include "cilk/cilk.h"
#include "cilk/cilk_api.h"
#include "prefix_sum.h"

#include <stdio.h>

void map (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2)) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (worker != NULL);

	// Define the grainsize
	//#pragma cilk grainsize = max(1024, min(nJob/(__cilkrts_get_nworkers()), 2048))
	cilk_for (size_t i = 0; i < nJob; i++)
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

void reduce (void *dest, void *src, size_t nJob,  size_t sizeJob,
		void (*worker)(void *v1, const void *v2, const void *v3)
{

}
void general_reduce (void *dest, void *src, size_t nJob,  size_t sizeJob,
		void (*worker)(void *v1, const void *v2, const void *v3),  size_t splitFacor)
{

	assert (dest != NULL);
	assert (src != NULL);
	assert (worker != NULL);



	if(splitFacor == 2)
		reduce(dest, src, nJob, sizeJob, worker);
	else{


		memcpy (dest, src, sizeJob);

		do{
			size_t nThreads = nJob / splitFacor;

			if(nThreads == 0){
				for (int i = 1;  i < nJob;  i++)
				   worker(dest, dest, src + i * sizeJob);
			}else{

				cilk_for(size_t currentThread = 0; currentThread < nThreads; j++){
					size_t lenght = slipFactor + ( currenThread < nJob%splitFactor ? 1 : 0);
					size_t offset = nJobs/nThreds + (currentThread < nJobs % nThreds ? currentThread : nJobs%Threds);
					for (int i = 1;  i < lenght;  i++)
						worker(dest + offset , dest + offset, (offset+i) * sizeJob);
					}
			}

		}while(nThreads > 1);
	}
}



void reduce_seq (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
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

void scan(void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (worker != NULL);
	
	// TODO resolve neutral element; wait for prof message
	double neutral_element = 0;
	
	prefix_sum(src, dest, nJob, sizeJob, worker, (void *) &neutral_element);
}

void scan_seq (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2, const void *v3)) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (worker != NULL);
	if (nJob > 1) {
		memcpy (dest, src, sizeJob);
		for (int i = 1;  i < nJob;  i++)
			worker(dest + i * sizeJob, src + i * sizeJob, dest + (i-1) * sizeJob);
	}
}


int pack (void* dest, void* src, size_t nJob, size_t sizeJob, const int* filter)
{


}


int pack_seq (void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
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
	assert (dest != NULL);
	assert (src != NULL);
	assert (filter != NULL);
	
	cilk_for (int i=0; i < nFilter; i++) {
		memcpy (dest + i * sizeJob, src + filter[i] * sizeJob, sizeJob);
	}
}

void gather_seq (void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter, int nFilter) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (filter != NULL);
	
	for (int i=0; i < nFilter; i++) {
		memcpy (dest + i * sizeJob, src + filter[i] * sizeJob, sizeJob);
	}
}

void scatter (void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (filter != NULL);
	
	cilk_for (int i=0; i < nJob; i++) {
		memcpy (dest + filter[i] * sizeJob, src + i * sizeJob, sizeJob);
	}
}

void scatter_seq (void *dest, void *src, size_t nJob, size_t sizeJob, const int *filter) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (filter != NULL);
	
	for (int i=0; i < nJob; i++) {
		memcpy (dest + filter[i] * sizeJob, src + i * sizeJob, sizeJob);
	}
}

void pipeline (void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (workerList != NULL);

	cilk_for(size_t i = 0; i < nJob; i++) {
		memcpy(dest + i*sizeJob, src + i*sizeJob, sizeJob);
	}

	// Start of the pipeline
	size_t limit = nWorkers-1;
	for(size_t i = 0; i < limit; i++) {
		// Compute each worker
		for( size_t j = 0; j <= i; j++) {
			void* job = dest + (i-j)*sizeJob;
			cilk_spawn workerList[j](job, job);
		}
		cilk_sync;
	}

	// Normal functioning of the pipeline
	limit = nJob;
	for(size_t i =  nWorkers-1; i < limit; i++) {
		// Compute each worker
		for( size_t j = 0; j < nWorkers; j++) {
			void* job = dest + (i-j)*sizeJob;
			cilk_spawn workerList[j](job, job);
		}
		cilk_sync;
	}

	// Finish of the ramaining tasks
	limit = nJob + nWorkers-1;
	for(size_t i = nJob; i < limit; i++) {
		// Compute each worker
		for( size_t j = i - nJob + 1; j < nWorkers; j++) {
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
		int limit = nWorkers-1;
		for(int i = 0; i < limit; i++) {
			// Compute each worker
			for( int j = 0; j <= i; j++) {
				size_t length = (i-j == nBatches-1) ? nJob-(nBatches-1)*nFarms : nFarms;

				cilk_for( int k = 0; k < length; k++) {
					void* job = dest + ((i-j)*nFarms+k)*sizeJob;
					workerList[j](job, job);
				}
			}
		}

		// Normal functioning of the pipeline
		limit = nBatches;
		for(int i =  nWorkers-1; i < limit; i++) {
			// Compute each worker
			for( int j = 0; j < nWorkers; j++) {
				size_t length = (i-j == nBatches-1) ? nJob-(nBatches-1)*nFarms : nFarms;

				cilk_for( int k = 0; k < length; k++) {
					void* job = dest + ((i-j)*nFarms+k)*sizeJob;
					workerList[j](job, job);
				}
			}
		}

		// Finish of the ramaining tasks
		limit = nBatches + nWorkers-1;
		for(int i = nBatches; i < limit; i++) {
			// Compute each worker
			for( int j = i - nBatches + 1; j < nWorkers; j++) {
				size_t length = (i-j == nBatches-1) ? nJob-(nBatches-1)*nFarms : nFarms;

				cilk_for( int k = 0; k < length; k++) {
					void* job = dest + ((i-j)*nFarms+k)*sizeJob;
					workerList[j](job, job);
				}
			}
		}
	}
}

void pipeline_seq (void *dest, void *src, size_t nJob, size_t sizeJob, void (*workerList[])(void *v1, const void *v2), size_t nWorkers) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (workerList != NULL);

	for (int i=0; i < nJob; i++) {
		memcpy (dest + i * sizeJob, src + i * sizeJob, sizeJob);
		for (int j = 0;  j < nWorkers;  j++)
			workerList[j](dest + i * sizeJob, dest + i * sizeJob);
	}
}

void farm (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers) {
	assert (dest != NULL);
	assert (src != NULL);
	assert (worker != NULL);

	cilk_for(int i = 0; i < nWorkers; i++){
		// Compute the amount of work each worker gets, distributing the remaining across multiple workers
		size_t batchSize = (nJob / nWorkers) + (( i < nJob % nWorkers) ? 1 : 0);

		// Compute the start of the worker's batch
		int start = i*(nJob / nWorkers) + (( i < nJob % nWorkers) ? i : nJob % nWorkers);

		// In each worker, execute serially each job on his batch
		for(int j = 0; j < batchSize; j++ ) {
			worker(dest + (start+j) * sizeJob, src + (start+j) * sizeJob);
		}
	}
}

void farm_seq (void *dest, void *src, size_t nJob, size_t sizeJob, void (*worker)(void *v1, const void *v2), size_t nWorkers) {
	map (dest, src, nJob, sizeJob, worker);
}
