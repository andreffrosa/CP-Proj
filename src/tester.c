#include <stdio.h>
#include <stdint.h>

#include <strings.h>
#include <stdlib.h>
#include <time.h>

#include "patterns.h"

#define TYPE double

typedef enum MODE_ {
	SEQ=0,
	PAR=1
} MODE;

void runTester(double*** result, size_t runs, size_t start, size_t max_size, size_t step);
double*** createResultsMatrix(size_t sizes, size_t functions);
void freeResultsMatrix(double*** results, size_t sizes, size_t functions);

static void workerAddOne(void* a, const void* b) {
    // a = b + 1
    *(TYPE *)a = *(TYPE *)b + 1;
}
static void workerMultTwo(void* a, const void* b) {
    // a = b * 2
    *(TYPE *)a = *(TYPE *)b * 2;
}

static void workerDivTwo(void* a, const void* b) {
    // a = b / 2
    *(TYPE *)a = *(TYPE *)b / 2;
}


//https://www.gnu.org/software/libc/manual/html_node/CPU-Time.html para colocar na bibliografia

unsigned long evalMap(void* src, void* dest, size_t nJob, size_t size, MODE mode) {
	clock_t start, end;
	unsigned long us_cpu_time_used;

	if( mode == SEQ) {
		start = clock();
		map_seq (dest, src, nJob, size, workerAddOne);
		end = clock();
	} else {
		start = clock();
		map (dest, src, nJob, size, workerAddOne);
		end = clock();
	}

	us_cpu_time_used = (unsigned long)((((double) (end - start)) / (CLOCKS_PER_SEC/ (1000*1000))) ); // in microseconds

	return us_cpu_time_used;
}

unsigned long evalPipeline (void* src, void* dest, size_t nJob, size_t size, MODE mode) {
    void (*pipelineFunction[])(void*, const void*) = {
        workerMultTwo,
        workerAddOne,
        workerDivTwo
    };
    int nPipelineFunction = sizeof (pipelineFunction)/sizeof(pipelineFunction[0]);

    clock_t start, end;
    	unsigned long us_cpu_time_used;

    	if( mode == SEQ) {
    		start = clock();
    	    pipeline_seq (dest, src, nJob, size, pipelineFunction, nPipelineFunction);
    		end = clock();
    	} else {
    		start = clock();
    	    pipeline (dest, src, nJob, size, pipelineFunction, nPipelineFunction);
    		end = clock();
    	}

    	us_cpu_time_used = (unsigned long)((((double) (end - start)) / (CLOCKS_PER_SEC/ (1000*1000))) ); // in microseconds

    	return us_cpu_time_used;
}

unsigned long evalFarm (void* src, void* dest, size_t nJob, size_t size, MODE mode) {
    clock_t start, end;
    	unsigned long us_cpu_time_used;

    	if( mode == SEQ) {
    		start = clock();
    		farm_seq (dest, src, nJob, size, workerAddOne, 3);
    		end = clock();
    	} else {
    		start = clock();
    		farm (dest, src, nJob, size, workerAddOne, 3);
    		end = clock();
    	}

    	us_cpu_time_used = (unsigned long)((((double) (end - start)) / (CLOCKS_PER_SEC/ (1000*1000))) ); // in microseconds

    	return us_cpu_time_used;
}

typedef unsigned long (*EVALFUNCTION)(void *, void*, size_t, size_t, MODE);

EVALFUNCTION evalFunction[] = {
    evalMap,
    /*testReduce,
    testScan,
    testPack,
    testGather,
    testScatter,*/
    evalPipeline,
    evalFarm
};


char *evalNames[] = {
    "Map",
    /*"testReduce",
    "testScan",
    "testPack",
    "testGather",
    "testScatter",*/
    "Pipeline",
    "Farm"
};

int nEvalFunctions = sizeof (evalFunction)/sizeof(evalFunction[0]);

// tester -n 10 -s 1000
int main(int argc, char** argv) {

	// processArgs

	size_t runs = 1;
	size_t step = 1;
	size_t start = 10000000;
	size_t max_size = 10000000;

	size_t sizes = ((max_size-start) / (double)step)+1;
	double*** results = createResultsMatrix(sizes, nEvalFunctions);

	runTester(results, runs, start, sizes, step);
	//saveResults();

	freeResultsMatrix(results, sizes, nEvalFunctions);

	return 0;
}

TYPE* createRandomArray(size_t n) {
	TYPE *src = malloc(sizeof(*src) * n);

	for (size_t i = 0; i < n; i++)
		src[i] = drand48();

	return src;
}

double*** createResultsMatrix(size_t sizes, size_t functions) {
	double*** results = malloc( sizes*sizeof(double**) );

	for(size_t i = 0; i < sizes; i++){
		results[i] = malloc( functions*sizeof(double*) );
		for(size_t j = 0; j < functions; j++) {
			results[i][j] = malloc(2*sizeof(double));
			bzero(results[i][j], 2*sizeof(double));
		}
	}
	return results;
}

void freeResultsMatrix(double*** results, size_t sizes, size_t functions) {
	for(size_t i = 0; i < sizes; i++){
		for(size_t j = 0; j < functions; j++) {
			free(results[i][j]);
		}
		free(results[i]);
	}
}

void runTester(double*** results, size_t runs, size_t start, size_t sizes, size_t step) {

	for(size_t i = 0; i < sizes; i++) {
		size_t current_size = i*step + start;
		//printf("Current_size: %lu\n", current_size);

		TYPE* src = createRandomArray(current_size);
		TYPE* dest = malloc (current_size*sizeof(TYPE));

		for(size_t f = 0; f < nEvalFunctions; f++) {
			//printf("Current pattern: %s\n", evalNames[f]);
			for(size_t run = 0; run < runs; run++) {
				printf("Size=%lu \t pattern=%s \t run=%lu/%lu \n", current_size, evalNames[f], run+1, runs);

				// Parallel
				unsigned long t;
				t = evalFunction[f](src, dest, current_size, sizeof(TYPE), PAR);
				//printf("parallel_%s %lu microseconds\n", evalNames[f], t);

				results[i][f][PAR] += t;

				// Seq
				t = evalFunction[f](src, dest, current_size, sizeof(TYPE), SEQ);
				//printf("sequential_%s %lu microseconds\n", evalNames[f], t);

				results[i][f][SEQ] += t;
			}
		}
		free(src);
		free(dest);
	}

	// Compute the average between the different runs
	if( runs > 1 ) {
		for(size_t i = 0; i < sizes; i++) {
			size_t current_size = i*step + start;
			printf("array size=%lu \t runs=%lu\n", current_size, runs );
			printf("Pattern \t\t\t Sequential 	\t Parallel\n");
			for(size_t j = 0; j < nEvalFunctions; j++){
				for(size_t k = 0; k < 2; k++){
					results[i][j][k] = results[i][j][k] / runs;
				}

				printf("%s \t\t\t %f us \t %f us \n", evalNames[j], results[i][j][SEQ], results[i][j][PAR]);
			}
			printf("\n");
		}
	}
}
