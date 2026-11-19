#ifndef BENCH_H
#define BENCH_H

double bench_once(int n, int threads, unsigned int seed, int repeats, double *residual);
int bench_table(int max_threads, unsigned int seed, int repeats);

#endif
