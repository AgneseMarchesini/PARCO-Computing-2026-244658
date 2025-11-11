// gcc -fopenmp -Wall -O3 .\src\sparse_matrix_x_vector.c .\src\mmio.c .\src\matrix.c -o spmv
// ./spmv matrix_file.mtx <mode> [chunk_size]
#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include <time.h>
#include "matrix.h"
#include <omp.h>
#include <string.h>
#include "timer.h"

int main(int argc, char *argv[]) {
    int i;
    
    if (argc != 5) {
        printf("Usage: %s matrix_file.mtx <mode> [chunk_size] [threads]\n", argv[0]);
        printf("<mode>: seq, static, dynamic, guided\n");
        printf("Chunk size for OpenMP scheduling\n");
        printf("Number of threads\n");
        return 1;
    }

    char* matrix_file = argv[1];
    char* mode = argv[2];
    int chunk_size = atoi(argv[3]);
    int threads = atoi(argv[4]);

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
    double time;

    // Untimed run to warm up caches
    if (strcmp(mode, "seq") == 0){
        matrix_vector_mul_sequential(&matrix, v, y);
    } else {
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
        matrix_vector_mul_parallel(&matrix, v, y);
    }

    for(i = 0; i < matrix.M; i++) {
        y[i] = 0.0;                     // Reset result vector y
    }

    // Timed run
    if (strcmp(mode, "seq") == 0){
        // Sequential execution
        GET_TIME(start);
        matrix_vector_mul_sequential(&matrix, v, y);
        GET_TIME(end);
        time = (end - start) * 1000.0; // milliseconds
    } else {
        // Parallel execution
        GET_TIME(start);
        matrix_vector_mul_parallel(&matrix, v, y);
        GET_TIME(end);
        time = (end - start) * 1000.0; // milliseconds
    }

    // Print the time for a single run
    printf("Time: %f\n", time);

    // Free memory
    matrix_free(&matrix);
    free(v);
    free(y);
    
    return 0;
}
