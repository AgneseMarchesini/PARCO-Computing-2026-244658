#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include <time.h>
#include "matrix.h"
#include <omp.h>

int compare_doubles(const void *a, const void *b) {
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

int main(int argc, char *argv[]) {
    
    if (argc < 3) {
        printf("Usage: %s matrix_file.mtx <mode> [num_runs]\n", argv[0]);
        printf("<mode>: seq, static, dynamic, guided\n");
        //[num_runs] is implemented to measure valgrind performance
        printf("[num_runs]: (optional) number of runs for benchmark, default: 10\n");
        return 1;
    }

    char* matrix_file = argv[1];
    char* mode = argv[2];
    int num_runs = 10;
    if (argc == 4) {
        num_runs = atoi(argv[3]);
    }

    SparseMatrixCSR matrix;

    matrix_load_from_mtx(argv[1], &matrix);
    
    double* v = (double*)malloc(matrix.N * sizeof(double));
    double* y = (double*)calloc(matrix.M, sizeof(double));

    // Initialize vector v with some random values
    srand((unsigned int)time(NULL));
    for(int i = 0; i < matrix.N; i++) {
        v[i] = (double)(rand() % 10);
    }

    double start, end;
    double times[num_runs];
    
    // Benchmarking loop
    double total_sum = 0.0; // To prevent compiler optimization
    for(int i =0; i < num_runs; i++){
        // Re-randomize v to avoid compiler optimization
        for(int j = 0; j < matrix.N; j++) {
            v[j] = (double)(rand() % 10);
        }

        start = omp_get_wtime();
        matrix_vector_mul(&matrix, v, y);
        end = omp_get_wtime();
        times[i] = (end - start) * 1000.0; // milliseconds
        printf("Run %d: %f milliseconds\n", i+1, times[i]);

        // Prevent compiler optimization (otherwise the time measurement are equal to 0)
        for (int j = 0; j < matrix.N; j++) {
            total_sum += y[j]; 
        }
    }

    /*
    // Debugging
    for(int i = 0; i < (matrix.M < 10 ? matrix.M : 10); i++) {
        printf("v[%d] = %f\n", i, v[i]);
    }

    for(int i = 0; i < (matrix.M < 10 ? matrix.M : 10); i++) {
        printf("y[%d] = %f\n", i, y[i]);
    }
    */

    // 90th percentile calculation
    qsort(times, num_runs, sizeof(double), compare_doubles);

    int p90_index = (int)(0.9 * num_runs) - 1; // index for 90th percentile

    printf("\n90th percentile time: %f ms\n", times[p90_index]);

    // Free memory
    matrix_free(&matrix);
    free(v);
    free(y);
    
    return 0;
}
