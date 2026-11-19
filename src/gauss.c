#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gauss.h"
#include "common.h"

#define EPS 1e-12

typedef struct {
    Matrix *m;
    int threads;
    int singular;
    int cancel;
    HANDLE start;
    SYNCHRONIZATION_BARRIER barrier;
} Shared;

typedef struct {
    Shared *sh;
    int id;
} WorkerArg;

static void choose_pivot(Shared *sh, int k){
    Matrix *m = sh->m;
    int best = k;
    double best_abs = fabs(m->rows[k][k]);
    for(int i = k + 1; i < m->n; i++){
        double a = fabs(m->rows[i][k]);
        if (a > best_abs){
            best_abs = a;
            best = i;
        }
    }
    if (best_abs < EPS){
        sh->singular = 1;
        return;
    }
    if (best != k){
        double *tmp = m->rows[k];
        m->rows[k] = m->rows[best];
        m->rows[best] = tmp;
    }
}

static void eliminate_rows(Matrix *m, int k, int id, int threads){
    const double *pivot = m->rows[k];
    int n = m->n;
    for(int i = k + 1 + id; i < n; i += threads){
        double *row = m->rows[i];
        double f = row[k] / pivot[k];
        if (f == 0.0){
            continue;
        }
        for(int j = k; j <= n; j++){
            row[j] -= f * pivot[j];
        }
    }
}

static DWORD WINAPI worker(LPVOID param){
    WorkerArg *arg = (WorkerArg *)param;
    Shared *sh = arg->sh;
    int n = sh->m->n;
    if (WaitForSingleObject(sh->start, INFINITE) != WAIT_OBJECT_0){
        print_error("WaitForSingleObject start");
        ExitProcess(EXIT_FAILURE);
    }
    if (sh->cancel){
        return 0;
    }
    for(int k = 0; k < n; k++){
        if (arg->id == 0){
            choose_pivot(sh, k);
        }
        EnterSynchronizationBarrier(&sh->barrier, 0);
        if (sh->singular){
            break;
        }
        eliminate_rows(sh->m, k, arg->id, sh->threads);
        EnterSynchronizationBarrier(&sh->barrier, 0);
    }
    return 0;
}

static int wait_workers(HANDLE *handles, int count){
    if (WaitForMultipleObjects((DWORD)count, handles, TRUE, INFINITE) == WAIT_OBJECT_0){
        return 0;
    }
    print_error("WaitForMultipleObjects");
    for(int i = 0; i < count; i++){
        if (WaitForSingleObject(handles[i], INFINITE) != WAIT_OBJECT_0){
            print_error("WaitForSingleObject worker");
            ExitProcess(EXIT_FAILURE);
        }
    }
    return -1;
}

int gauss_solve(Matrix *m, int threads, double *x, DWORD *tids){
    int n = m->n;
    int result = GAUSS_SYSERROR;
    int created = 0;
    int joined = 0;
    Shared sh = {0};
    HANDLE *handles = NULL;
    WorkerArg *args = NULL;

    sh.m = m;
    sh.threads = threads;
    sh.singular = 0;

    if (threads < 1 || threads > MAX_THREADS){
        fprintf(stderr, "ERROR: invalid worker count\n");
        return GAUSS_SYSERROR;
    }

    if (!InitializeSynchronizationBarrier(&sh.barrier, threads, -1)){
        print_error("InitializeSynchronizationBarrier");
        return GAUSS_SYSERROR;
    }

    sh.start = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (sh.start == NULL){
        print_error("CreateEventA start");
        goto cleanup;
    }

    handles = malloc((size_t)threads * sizeof(HANDLE));
    args = malloc((size_t)threads * sizeof(WorkerArg));
    if (handles == NULL || args == NULL){
        fprintf(stderr, "ERROR: malloc failed\n");
        goto cleanup;
    }

    for(created = 0; created < threads; created++){
        DWORD tid;
        args[created].sh = &sh;
        args[created].id = created;
        handles[created] = CreateThread(NULL, 0, worker, &args[created], 0, &tid);
        if (handles[created] == NULL){
            print_error("CreateThread");
            goto cleanup;
        }
        if (tids != NULL){
            tids[created] = tid;
        }
    }

    if (!SetEvent(sh.start)){
        print_error("SetEvent start");
        ExitProcess(EXIT_FAILURE);
    }

    int wait_result = wait_workers(handles, threads);
    joined = 1;
    if (wait_result != 0){
        goto cleanup;
    }

    if (sh.singular){
        result = GAUSS_SINGULAR;
        goto cleanup;
    }

    for(int i = n - 1; i >= 0; i--){
        double s = m->rows[i][n];
        for(int j = i + 1; j < n; j++){
            s -= m->rows[i][j] * x[j];
        }
        x[i] = s / m->rows[i][i];
    }
    result = GAUSS_OK;

cleanup:
    if (created > 0 && !joined){
        sh.cancel = 1;
        if (!SetEvent(sh.start)){
            print_error("SetEvent cancel");
            ExitProcess(EXIT_FAILURE);
        }
        if (wait_workers(handles, created) != 0){
            result = GAUSS_SYSERROR;
        }
    }
    for(int i = 0; i < created; i++){
        if (!CloseHandle(handles[i])){
            print_error("CloseHandle worker");
            result = GAUSS_SYSERROR;
        }
    }
    if (sh.start != NULL && !CloseHandle(sh.start)){
        print_error("CloseHandle start");
        result = GAUSS_SYSERROR;
    }
    free(handles);
    free(args);
    if (!DeleteSynchronizationBarrier(&sh.barrier)){
        print_error("DeleteSynchronizationBarrier");
        result = GAUSS_SYSERROR;
    }
    return result;
}
