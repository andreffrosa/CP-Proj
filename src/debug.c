#include <stdio.h>
#include <unistd.h>
#include "debug.h"


int debug = 0;


void printDouble (const double *src, size_t n, const char *msg) {
    if (debug) {
        printf ("%s %s: ", msg, "double");
        for (int i = 0;  i < n;  i++)
            printf ("[%d]=%lf ", i, src[i]);
        printf ("\n-------------------------------\n");
    }
}

void printInt (const int *src, size_t n, const char *msg) {
    if (debug) {
        printf ("%s int: ", msg);
        for (int i = 0;  i < n;  i++)
            printf ("[%d]=%d ", i, src[i]);
        printf ("\n-------------------------------\n");
    }
}
