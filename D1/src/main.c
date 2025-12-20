// gcc -fopenmp -Wall -O3 .\src\sparse_matrix_x_vector.c .\src\mmio.c .\src\matrix.c -o spmv
// ./spmv matrix_file.mtx <mode> [chunk_size] [threads] [number_of_runs]
#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include <time.h>
#include "matrix.h"
#include <omp.h>
#include <string.h>
#include "timer.h"
#include <math.h>

int compare_doubles(const void *a, const void *b) {
    double double_a = *(const double *)a;
    double double_b = *(const double *)b;

    if (double_a < double_b) return -1;
    if (double_a > double_b) return 1;
    return 0;
}

int main(int argc, char *argv[]) {
    int i, j;
    
    if (argc != 6) {
        printf("Usage: %s matrix_file.mtx <mode> [chunk_size] [threads]\n", argv[0]);
        printf("<mode>: seq, static, dynamic, guided\n");
        printf("Chunk size for OpenMP scheduling\n");
        printf("Number of threads\n");
        printf("Number of runs\n");
        return 1;
    }

    char* matrix_file = argv[1];
    char* mode = argv[2];
    int chunk_size = atoi(argv[3]);
    int threads = atoi(argv[4]);
    int num_runs = atoi(argv[5]);

    SparseMatrixCSR matrix;

    if (matrix_load_from_mtx(matrix_file, &matrix) != 0) {
        fprintf(stderr, "CRITICAL ERROR: Failed to load matrix: %s\n", matrix_file);
        fprintf(stderr, "Check if the 'data' directory and .mtx file exist.\n");
        return 1;
    }
    
    double* v = (double*)malloc(matrix.N * sizeof(double));
    double* y = (double*)calloc(matrix.M, sizeof(double));

    if (v == NULL || y == NULL) {
        fprintf(stderr, "CRITICAL ERROR: Malloc failed. (Matrix M/N: %d, %d)\n", matrix.M, matrix.N);
        return 1;
    }

    // Initialize vector v with some random values
    srand((unsigned int)time(NULL));
    for(i = 0; i < matrix.N; i++) {
        v[i] = (double)(rand() % 10);
    }

    double start, end;
    double time[num_runs];

    for(i = 0; i < num_runs; i++){
        for(j = 0; j < matrix.M; j++) {
            y[j] = 0.0;                     // Reset result vector y
        }

        if (strcmp(mode, "seq") == 0){
            // Sequential execution
            GET_TIME(start);
            matrix_vector_mul_sequential(&matrix, v, y);
            GET_TIME(end);
            time[i] = (end - start) * 1000.0; // milliseconds
        } else {
            // Parallel execution
            omp_set_num_threads(threads);
            if(strcmp(mode, "static") == 0){
                omp_set_schedule(omp_sched_static, chunk_size);
            } else if(strcmp(mode, "dynamic") == 0){
                omp_set_schedule(omp_sched_dynamic, chunk_size);
            } else if(strcmp(mode, "guided") == 0){
                omp_set_schedule(omp_sched_guided, chunk_size);
            } else {
                fprintf(stderr, "Unknown mode: %s\n", mode);
                return 1;
            }
            
            GET_TIME(start);
            matrix_vector_mul_parallel(&matrix, v, y);
            GET_TIME(end);
            time[i] = (end - start) * 1000.0; // milliseconds
        }
    }

    qsort(time, num_runs, sizeof(double), compare_doubles);

    int index_90 = (int)ceil(0.9 * num_runs)-1;

    //print 90th percentile time in milliseconds
    printf("Time: %f", time[index_90]);

    // Free memory
    matrix_free(&matrix);
    free(v);
    free(y);
    
    return 0;
}
