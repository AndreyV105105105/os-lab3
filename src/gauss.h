#ifndef GAUSS_H
#define GAUSS_H

#include <windows.h>
#include "matrix.h"

#define MAX_THREADS MAXIMUM_WAIT_OBJECTS

#define GAUSS_OK        0
#define GAUSS_SINGULAR  1
#define GAUSS_SYSERROR -1

int gauss_solve(Matrix *m, int threads, double *x, DWORD *tids);

#endif
