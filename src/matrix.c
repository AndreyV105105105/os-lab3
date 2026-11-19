#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "matrix.h"

int matrix_alloc(Matrix *m, int n){
    m->n = n;
    m->data = malloc((size_t)n * (size_t)(n + 1) * sizeof(double));
    m->rows = malloc((size_t)n * sizeof(double *));
    if (m->data == NULL || m->rows == NULL){
        free(m->data);
        free(m->rows);
        m->data = NULL;
        m->rows = NULL;
        return -1;
    }
    for(int i = 0; i < n; i++){
        m->rows[i] = m->data + (size_t)i * (size_t)(n + 1);
    }
    return 0;
}

void matrix_free(Matrix *m){
    free(m->data);
    free(m->rows);
    m->data = NULL;
    m->rows = NULL;
}

void matrix_random(Matrix *m, unsigned int seed){
    srand(seed);
    for(int i = 0; i < m->n; i++){
        for(int j = 0; j <= m->n; j++){
            m->rows[i][j] = (double)rand() / RAND_MAX * 20.0 - 10.0;
        }
    }
}

void matrix_copy(Matrix *dst, const Matrix *src){
    for(int i = 0; i < src->n; i++){
        memcpy(dst->rows[i], src->rows[i], (size_t)(src->n + 1) * sizeof(double));
    }
}

int matrix_read_file(Matrix *m, const char *path){
    FILE *f = fopen(path, "r");
    int n;
    if (f == NULL){
        perror(path);
        return -1;
    }
    if (fscanf(f, "%d", &n) != 1 || n <= 0){
        fprintf(stderr, "%s: first number must be n > 0\n", path);
        fclose(f);
        return -1;
    }
    if (matrix_alloc(m, n) != 0){
        fprintf(stderr, "Not enough memory for n = %d\n", n);
        fclose(f);
        return -1;
    }
    for(int i = 0; i < n; i++){
        for(int j = 0; j <= n; j++){
            if (fscanf(f, "%lf", &m->rows[i][j]) != 1){
                fprintf(stderr, "%s: expected %d numbers in row %d\n", path, n + 1, i + 1);
                matrix_free(m);
                fclose(f);
                return -1;
            }
        }
    }
    fclose(f);
    return 0;
}

double matrix_residual(const Matrix *orig, const double *x){
    double max_err = 0.0;
    for(int i = 0; i < orig->n; i++){
        double s = 0.0;
        for(int j = 0; j < orig->n; j++){
            s += orig->rows[i][j] * x[j];
        }
        double err = fabs(s - orig->rows[i][orig->n]);
        if (err > max_err){
            max_err = err;
        }
    }
    return max_err;
}
