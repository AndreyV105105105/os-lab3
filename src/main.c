#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "matrix.h"
#include "gauss.h"
#include "bench.h"

static void usage(void){
    fprintf(stderr,
            "Usage: gauss.exe -t THREADS (-n N | -f FILE | -b) [-s SEED] [-r REPEATS]\n"
            "  -t  max number of worker threads (1..%d)\n"
            "  -n  solve random N x N system\n"
            "  -f  solve system from file (n, then n rows of n+1 numbers)\n"
            "  -b  benchmark: speedup/efficiency table\n", MAX_THREADS);
}

static int parse_positive(const char *s){
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v <= 0 || v > 1000000){
        return -1;
    }
    return (int)v;
}

int main(int argc, char *argv[]){
    int threads = -1, n = -1, repeats = 1, bench = 0;
    unsigned int seed = 42;
    const char *file = NULL;
    Matrix orig = {0}, work = {0};
    double *x = NULL;
    DWORD tids[MAX_THREADS];
    int result = EXIT_FAILURE;
    double best = -1.0;
    int rc = GAUSS_OK;

    for(int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-b") == 0){
            bench = 1;
        }
        else if (i + 1 < argc && strcmp(argv[i], "-t") == 0){
            threads = parse_positive(argv[++i]);
        }
        else if (i + 1 < argc && strcmp(argv[i], "-n") == 0){
            n = parse_positive(argv[++i]);
        }
        else if (i + 1 < argc && strcmp(argv[i], "-r") == 0){
            repeats = parse_positive(argv[++i]);
        }
        else if (i + 1 < argc && strcmp(argv[i], "-s") == 0){
            seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        }
        else if (i + 1 < argc && strcmp(argv[i], "-f") == 0){
            file = argv[++i];
        }
        else{
            usage();
            return EXIT_FAILURE;
        }
    }
    if (threads < 1 || threads > MAX_THREADS || repeats < 1){
        fprintf(stderr, "ERROR: -t must be 1..%d, -r must be >= 1\n", MAX_THREADS);
        usage();
        return EXIT_FAILURE;
    }

    printf("[PID %lu] Gauss method, max threads = %d\n",
           (unsigned long)GetCurrentProcessId(), threads);

    if (bench){
        return bench_table(threads, seed, repeats) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (file != NULL){
        if (matrix_read_file(&orig, file) != 0){
            return EXIT_FAILURE;
        }
    }
    else if (n > 0){
        if (matrix_alloc(&orig, n) != 0){
            fprintf(stderr, "ERROR: not enough memory for n = %d\n", n);
            return EXIT_FAILURE;
        }
        matrix_random(&orig, seed);
    }
    else{
        usage();
        return EXIT_FAILURE;
    }
    n = orig.n;
    if (threads > n){
        threads = n;
    }

    if (matrix_alloc(&work, n) != 0 || (x = malloc((size_t)n * sizeof(double))) == NULL){
        fprintf(stderr, "ERROR: not enough memory for n = %d\n", n);
        goto cleanup;
    }

    for(int r = 0; r < repeats; r++){
        matrix_copy(&work, &orig);
        double t0 = now_ms();
        rc = gauss_solve(&work, threads, x, tids);
        double t = now_ms() - t0;
        if (rc != GAUSS_OK){
            break;
        }
        if (best < 0 || t < best){
            best = t;
        }
    }
    if (rc == GAUSS_SINGULAR){
        printf("Matrix is singular: the system has no unique solution\n");
        result = 2;
        goto cleanup;
    }
    if (rc != GAUSS_OK){
        goto cleanup;
    }

    printf("n = %d, threads = %d\nworker TIDs:", n, threads);
    for(int i = 0; i < threads; i++){
        printf(" %lu", (unsigned long)tids[i]);
    }
    printf("\ntime = %.2f ms\n", best);
    printf("residual max|Ax - b| = %.2e\n", matrix_residual(&orig, x));
    if (n <= 10){
        for(int i = 0; i < n; i++){
            printf("x%d = %.6f\n", i + 1, x[i]);
        }
    }
    result = EXIT_SUCCESS;

cleanup:
    free(x);
    matrix_free(&work);
    matrix_free(&orig);
    return result;
}
