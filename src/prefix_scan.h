#ifndef __PREFIX_SCAN_H
#define __PREFIX_SCAN_H

/*
 * Execute prefix sum algorithm, given arrays of input and output of size n_jobs, where each array element is of size size_job.
 * The operation to be applied in the prefix scan sould also be specified, as welll as the neutral element peratining to the operation
 * TODO remove neutral element requirement
 */
void prefix_scan(void *input, void *output, size_t n_jobs, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3), void *neutral_element);

#endif