#include <stdio.h>
#include <stdint.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

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

static void processArgs(int argc, char** argv, size_t* runs, size_t* step, size_t* start, size_t* n_steps);
void saveResults(double*** results, size_t x_base, size_t y_base, size_t start, size_t n_steps, char* filePattern);

// tester -n 10 -s 1000
int main(int argc, char** argv) {

	// Default values
	size_t runs = 1;
	size_t step = 10000;
	size_t start = 10000;
	size_t n_steps = 10;

	// Initialize arguments
	processArgs(argc, argv, &runs, &step, &start, &n_steps);

	printf("runs=%lu \t step=%lu \t start=%lu \t n_steps=%lu \n", runs, step, start, n_steps);

	//size_t sizes = ((n_steps-start) / (double)step)+1;
	double*** results = createResultsMatrix(n_steps, nEvalFunctions);

	runTester(results, runs, start, n_steps, step);

	saveResults(results, 1, step, start, n_steps, "./plots/%s.csv");

	freeResultsMatrix(results, n_steps, nEvalFunctions);

	return 0;
}

void saveResults(double*** results, size_t y_base, size_t step, size_t start, size_t n_steps, char* filePattern) {

	FILE * fp;

	for( size_t pattern = 0; pattern < nEvalFunctions; pattern++) {
		// Compute file name
		char fileName[strlen(filePattern)+strlen(evalNames[pattern])+1];
		sprintf(fileName, filePattern, evalNames[pattern]);

		fp = fopen (fileName, "w");

		fprintf (fp, ";%s;%s\n", "sequential", "parallel");
		int x_min = start / step;
		int x_max = (start / step) + n_steps;
		for( int x = x_min; x < x_max; x++) {
			fprintf (fp, "%d;%f;%f\n", x, results[x-x_min][pattern][SEQ], results[x-x_min][pattern][PAR]);
		}

		fclose (fp);
	}
}

static void processArgs(int argc, char** argv, size_t* runs, size_t* step, size_t* start, size_t* n_steps) {
	int c;

	opterr = 0;

	while ((c = getopt(argc, argv, "r:s:i:n:")) != -1)
		switch (c) {
		case 'r':
			*runs = strtol (optarg, NULL, 10);;
			break;
		case 's':
			*step = strtol (optarg, NULL, 10);
			break;
		case 'i':
			*start = strtol (optarg, NULL, 10);
			break;
		case 'n':
			*n_steps = strtol (optarg, NULL, 10);
			break;
		case '?':
			if (optopt == 'r' || optopt == 's' || optopt == 'i' || optopt == 'n' )
				fprintf(stderr, "Option -%c is followed a the number.\n", optopt);
			/*else if (isprint(optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);*/
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			break;
		}
}

TYPE* createRandomArray(size_t n) {
	TYPE *src = malloc(sizeof(*src) * n);

	for (size_t i = 0; i < n; i++)
		src[i] = drand48();

	return src;
}

double*** createResultsMatrix(size_t n_steps, size_t functions) {
	double*** results = malloc( n_steps*sizeof(double**) );

	for(size_t i = 0; i < n_steps; i++){
		results[i] = malloc( functions*sizeof(double*) );
		for(size_t j = 0; j < functions; j++) {
			results[i][j] = malloc(2*sizeof(double));
			bzero(results[i][j], 2*sizeof(double));
		}
	}
	return results;
}

void freeResultsMatrix(double*** results, size_t n_steps, size_t functions) {
	for(size_t i = 0; i < n_steps; i++){
		for(size_t j = 0; j < functions; j++) {
			free(results[i][j]);
		}
		free(results[i]);
	}
}

void runTester(double*** results, size_t runs, size_t start, size_t n_steps, size_t step) {

	for(size_t i = 0; i < n_steps; i++) {
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
	if( runs >= 1 ) {
		for(size_t i = 0; i < n_steps; i++) {
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
