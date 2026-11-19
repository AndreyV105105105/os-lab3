#include <stdio.h>
#include <stdlib.h>
#include "bench.h"
#include "gauss.h"
#include "common.h"

double bench_once(int n, int threads, unsigned int seed, int repeats, double *residual){
    Matrix orig = {0}, work = {0};
    double *x = NULL;
    double best = -1.0;

    if (matrix_alloc(&orig, n) != 0 || matrix_alloc(&work, n) != 0 ||
        (x = malloc((size_t)n * sizeof(double))) == NULL){
        fprintf(stderr, "ERROR: not enough memory for n = %d\n", n);
        goto cleanup;
    }
    matrix_random(&orig, seed);

    for(int r = 0; r < repeats; r++){
        matrix_copy(&work, &orig);
        double t0 = now_ms();
        int rc = gauss_solve(&work, threads, x, NULL);
        double t = now_ms() - t0;
        if (rc != GAUSS_OK){
            fprintf(stderr, "ERROR: gauss_solve failed (n = %d, threads = %d)\n", n, threads);
            best = -1.0;
            goto cleanup;
        }
        if (best < 0 || t < best){
            best = t;
        }
    }
    *residual = matrix_residual(&orig, x);

cleanup:
    free(x);
    matrix_free(&work);
    matrix_free(&orig);
    return best;
}

int bench_table(int max_threads, unsigned int seed, int repeats){
    static const int sizes[] = {200, 500, 1000, 2000};
    static const int thread_list[] = {1, 2, 4, 6, 8, 12, 16, 24, 32, 48, 64};
    SYSTEM_INFO si;

    GetSystemInfo(&si);
    printf("Logical processors: %lu, max threads: %d, repeats: %d\n\n",
           (unsigned long)si.dwNumberOfProcessors, max_threads, repeats);
    printf("%6s %8s %12s %9s %11s %10s\n",
           "n", "threads", "time_ms", "speedup", "efficiency", "residual");

    for(size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++){
        double t1 = -1.0;
        for(size_t k = 0; k < sizeof(thread_list) / sizeof(thread_list[0]); k++){
            int p = thread_list[k];
            double res;
            if (p > max_threads){
                break;
            }
            double t = bench_once(sizes[s], p, seed, repeats, &res);
            if (t < 0){
                return -1;
            }
            if (p == 1){
                t1 = t;
            }
            double speedup = t1 / t;
            double efficiency = speedup / p;
            printf("%6d %8d %12.2f %9.2f %11.2f %10.1e\n",
                   sizes[s], p, t, speedup, efficiency, res);
            fflush(stdout);
        }
        printf("\n");
    }
    return 0;
}
