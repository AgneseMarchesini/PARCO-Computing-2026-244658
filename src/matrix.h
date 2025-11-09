#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>

typedef struct {
    int M;          // Number of rows
    int N;          // Number of columns
    int nz;         // Number of non-zero entries

    int* row_pnt;   // Row indices of non-zero entries CSR
    int* col_pnt;   // Column indices of non-zero entries CSR
    double* val_pnt;// Values of non-zero entries CSR
} SparseMatrixCSR;

// Load a sparse matrix from a Matrix Market (.mtx) file into CSR format
int matrix_load_from_mtx(const char* filename, SparseMatrixCSR* matrix);

// Free the memory allocated for the sparse matrix
void matrix_free(SparseMatrixCSR* matrix);

// Multiply the sparse matrix by a dense vector
void matrix_vector_mul_sequential(const SparseMatrixCSR* matrix, const double* v, double* y);

// Multiply the sparse matrix by a dense vector in parallel
void matrix_vector_mul_parallel(const SparseMatrixCSR* matrix, const double* v, double* y);

#endif
