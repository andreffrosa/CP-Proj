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
	#pragma cilk grainsize = max(1024, min(nJob/(__cilkrts_get_nworkers()), 2048))
    cilk_for (int i=0; i < nJob; i++) {
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
    memcpy(dest, src, nJob*sizeJob);

    /*printf("[%f", *((double*)src));
    for(int i = 1; i < nJob; i++)
    	printf(", %f", *((double*)(src + i*sizeJob)));
    printf("]\n");*/

    //size_t batch_size = nJob % nWorkers == 0 ? nJob / nWorkers : (nJob / nWorkers)+1; // Verificar se é divisivel. +1 não é a melhor opção
    size_t batch_size = nJob / nWorkers;

    for(int i=0; i < nWorkers + (nWorkers-1); i++) { // como descobrir o max i?
    	int j = min(i, nWorkers-1); // Isto está bem? min(i, nWorkers-1) ?
    	int limit = i - min(i, nWorkers-1);
    	printf("%d <= j <= %d\n", limit, j);
    	for(; j >= limit; j--) {
    		printf("j=%d\n", j);
    		int index = min(i, nWorkers-1 ) - j;
    		int start = index*batch_size + limit*batch_size;
    		//int limit2 = min(batch_size, nJob-start);
    		int limit2 = ( (nJob) - (start+batch_size)  < batch_size ) ? nJob-start : batch_size;
    		printf("%d <= k < %d\n", start, start + limit2);
    		/*cilk_for(int k = 0; k < limit2; k++) {
    			workerList[j](dest + start*sizeJob + k * sizeJob, dest + start*sizeJob + k * sizeJob);
    		}*/
    		map(dest + start*sizeJob, dest + start*sizeJob, limit2-start, sizeJob, workerList[j]);
    	}
        /*printf("[%f", *((double*)dest));
        for(int i = 1; i < nJob; i++)
        	printf(", %f", *((double*)(dest + i*sizeJob)));
        printf("]\n");*/

    	}/*
    for (int i=0; i < nJob; i++) {
                    memcpy (dest + i * sizeJob, src + i * sizeJob, sizeJob);
                    for (int j = 0;  j < nWorkers;  j++)
                        workerList[j](dest + i * sizeJob, dest + i * sizeJob);
                }
    printf("[%f", *((double*)dest));
            for(int i = 1; i < nJob; i++)
            	printf(", %f", *((double*)(dest + i*sizeJob)));
            printf("]\n");*/
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
