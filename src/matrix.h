#ifndef MATRIX_H
#define MATRIX_H

typedef struct {
    int n;
    double *data;
    double **rows;
} Matrix;

int matrix_alloc(Matrix *m, int n);
void matrix_free(Matrix *m);
void matrix_random(Matrix *m, unsigned int seed);
void matrix_copy(Matrix *dst, const Matrix *src);
int matrix_read_file(Matrix *m, const char *path);
double matrix_residual(const Matrix *orig, const double *x);

#endif
