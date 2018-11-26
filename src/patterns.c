#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "patterns.h"
#include "cilk/cilk.h"
#include "cilk/cilk_api.h"
#include "prefix_scan.h"

#include <stdio.h>

#define TYPE int
#define FMT "%lf"
#define SUM_NEUTRAL 0.0
#define MULT_NEUTRAL 1.0


//custom workers
static void customWorkerAdd(void* a, const void* b, const void* c) {

	TYPE res_b = b == NULL ? SUM_NEUTRAL : *(TYPE *)b;
	TYPE res_c = c == NULL ? SUM_NEUTRAL : *(TYPE *)c;

   // a = b + c
    *(TYPE *)a = res_b + res_c;
}


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
		void (*worker)(void *v1, const void *v2, const void *v3))
{
//	tilled_reduce(dest, src, nJob, sizeJob, worker, 3);
//	printf("Resultado tilled: \n");
//	printf("%lf\n", *((double*) dest));

	reduce_seq(dest, src, nJob, sizeJob, worker);
//	printf("Resultado seq:\n");
//	printf("%lf\n", *((double*) dest));

}
void tilled_reduce (void *dest, void *src, size_t nJob,  size_t sizeJob,
		void (*worker)(void *v1, const void *v2, const void *v3),  size_t tilleSize)
{

	assert (dest != NULL);
	assert (src != NULL);
	assert (worker != NULL);

	if(tilleSize == 2)
		reduce(dest, src, nJob, sizeJob, worker);
	else{

		size_t nThreads;
		size_t currentSize = nJob;
		void * aux = malloc((nJob/tilleSize) * sizeJob);
		void * read = src;
		void * write = dest;

		do{

			nThreads = currentSize / tilleSize;
			if(nThreads == 0)
				nThreads = 1;

			/*printf("Print src: \n");
				for(size_t k = 0; k < nJob; k++){
					printf("%lf ", ((double*) src)[k]);
				}
*/
			cilk_for(size_t currentThread = 0; currentThread < nThreads; currentThread++){
				size_t lenght = tilleSize + ( currentThread < currentSize%tilleSize ? 1 : 0);
				size_t offset = tilleSize * currentThread + (currentThread < currentSize % tilleSize ? currentThread : currentSize%tilleSize);
				memcpy(write + currentThread*sizeJob, read + offset*sizeJob, sizeJob);

				for (int i = 1;  i < lenght;  i++){
//					size_t toPrint = offset +i;
//					printf("Offeset + i: %zu \n",toPrint);
					worker( write + currentThread*sizeJob, write + currentThread*sizeJob, read + (offset +i)*sizeJob);

				}
			}
/*
			printf("\n");

			printf("Print write: \n");
			for(size_t y = 0; y < nThreads; y++){
				printf("%lf ", ((double*) write)[y]);
			}
			printf("\n");

*/
			currentSize = nThreads;
			read = read == dest ? aux : dest;
			write = write == dest && currentSize > tilleSize ? aux : dest;

		}while(nThreads > 1);

		free(aux);
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
	
	prefix_scan(src, dest, nJob, sizeJob, worker);
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


int split(void* dest, void* src, size_t nJob, size_t sizeJob, const int* filter)
{
	assert (dest != NULL);
	assert (src != NULL);
	assert (filter != NULL);

	//inverting mask
	int * invertedFilter = malloc( nJob * sizeof(int) );
	cilk_for(size_t i = 0; i < nJob; i++){
		invertedFilter[i] = 1 - filter[i];
	}

	//calculating positive and negative bitsums
	int * negativesBitSum = malloc( nJob * sizeof(int) );
	cilk_spawn scan( (void*) negativesBitSum, invertedFilter, nJob, sizeof(int) ,customWorkerAdd);

	int * positivesBitSum = malloc( nJob * sizeof(int) );
	scan( (void*) positivesBitSum, (void *) filter, nJob, sizeof(int), customWorkerAdd);

	cilk_sync;

	//packing values
	size_t offset = positivesBitSum[nJob-1];


	cilk_for(size_t i = 0; i < nJob; i++){
		size_t pos =  filter[i] ? positivesBitSum[i] -1 : negativesBitSum[i]-1 + offset;
		memcpy( dest + pos*sizeJob, src + i*sizeJob, sizeJob);
	}

	free(positivesBitSum);
	free(negativesBitSum);
	free(invertedFilter);

	return offset;
}




int pack (void* dest, void* src, size_t nJob, size_t sizeJob, const int* filter)
{
	assert (dest != NULL);
	assert (src != NULL);
	assert (filter != NULL);

	//allocating memory for bitsum
	int *bitSum = malloc( nJob * sizeof(int) );

	//calculate bitsum
	scan( (void*) bitSum, (void *) filter, nJob, sizeof(int), customWorkerAdd);

	cilk_for(size_t i = 0; i < nJob; i++){
		if(filter[i]){
			size_t pos = bitSum[i] -1;
			memcpy( dest + pos*sizeJob, src + i*sizeJob, sizeJob);
		}
	}

	int returnVal = bitSum[nJob-1]-1;
	free(bitSum);

	return returnVal;
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






