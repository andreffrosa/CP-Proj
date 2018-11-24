#ifndef __PREFIX_SUM_H
#define __PREFIX_SUM_H

/*
 * Execute prefix sum algorithm, given arrays of input and output of size n_jobs, where each array element is of size size_job.
 * The operation to be applied in the prefix sum sould also be specified, as welll as the neutral element peratining to the operation.
 */
void prefix_sum(void *input, void *output, size_t n_jobs, size_t size_job, void (*worker)(void *v1, const void *v2, const void *v3), void *neutral_element);

#endif