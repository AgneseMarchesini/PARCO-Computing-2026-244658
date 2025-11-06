#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include <time.h>
#include "matrix.h"

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        printf("Usage: %s matrix_file.mtx\n", argv[0]);
        return 1;
    }

    SparseMatrixCSR matrix;

    matrix_load_from_mtx(argv[1], &matrix);
    
    double* v = (double*)malloc(matrix.N * sizeof(double));
    double* y = (double*)calloc(matrix.M, sizeof(double));

    // Initialize vector v with some values
    for(int i = 0; i < matrix.N; i++) {
        v[i] = (double)(rand() % 10);
    }
    
    printf("Matrix x vector multiplication...\n");
    clock_t start, end;
	double elapsed_seconds;
    
    start = clock();
    matrix_vector_mul(&matrix, v, y);
    end = clock();

    for(int i = 0; i < (matrix.M < 10 ? matrix.M : 10); i++) {
        printf("v[%d] = %f\n", i, v[i]);
    }

    for(int i = 0; i < (matrix.M < 10 ? matrix.M : 10); i++) {
        printf("y[%d] = %f\n", i, y[i]);
    }

	elapsed_seconds = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\n	Time taken: %f seconds\n", elapsed_seconds);

    // Free memory
    matrix_free(&matrix);
    free(v);
    free(y);
    
    return 0;
}
