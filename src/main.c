#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

#include "unit.h"
#include "debug.h"

#define TYPE double


static long long wall_clock_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1e6 + tv.tv_usec;
}



int main(int argc, char* argv[]) {
    int i, N;
    long long start, end;
    

    int c;
    while ((c = getopt (argc, argv, "d")) != -1)
    switch (c) {
        case 'd':
            debug = 1; break;
        default:
            printf("Invalid option\n");
            abort ();
    }
    argc -= optind;
    argv += optind;
    
    if (argc != 1) {
        printf("Usage: ./example N\n");
        return -1;
    }

    srand(time(NULL));
    srand48(time(NULL));

    N = atol(argv[0]);


    printf ("Initializing SRC array\n");
    TYPE *src = malloc (sizeof(*src) * N);
    for (i = 0; i < N; i++)
        src[i] = drand48();
    printf ("Done!\n");
    
    printDouble (src, N, "SRC");
    if (debug)
        printf ("\n\n");

    for (int i = 0;  i < nTestFunction;  i++) {
        start = wall_clock_time();
        testFunction[i] (src, N, sizeof(*src));
        end = wall_clock_time();
        printf ("%s:\t%8ld\tmicroseconds\n", testNames[i], (long) (end-start));
        if (debug)
            printf ("\n\n");
    }

    return 0;
}
