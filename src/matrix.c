#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mmio.h"
#include "matrix.h"
#include <string.h>

// Temporary container for (row, column, value) triplet
typedef struct {
    int row;
    int col;
    double val;
} Triplet;

// Load a sparse matrix from a Matrix Market (.mtx) file into CSR format
int matrix_load_from_mtx(const char* filename, SparseMatrixCSR* matrix) {
    FILE *f;
    MM_typecode matcode;
    int M, N, nz;            // rows, cols, nonzeros
    int i;
    Triplet* triplets;

    // Open the file
    if ((f = fopen(filename, "r")) == NULL) {
        perror("fopen");
        return 1;
    }

    // Read the banner (first line)
    if (mm_read_banner(f, &matcode) != 0) {
        printf("Could not process Matrix Market banner.\n");
        return 1;
    }

    // Read the size (rows, cols, nnz)
    if (mm_read_mtx_crd_size(f, &M, &N, &nz) != 0)
        return 1;

    // Allocate space
    triplets = (Triplet*) malloc(nz * sizeof(Triplet));
    if(triplets == NULL) {
        printf("Error malloc");
        fclose(f);
        return 1;
    }

    // Read triplets
    for (i = 0; i < nz; i++) {
        fscanf(f, "%d %d %lf", &triplets[i].row, &triplets[i].col, &triplets[i].val);
        triplets[i].row--;  // convert to 0-based indexing (C uses 0-based)
        triplets[i].col--;
    }

    fclose(f);

    // Popolate matrix
    matrix->M = M;
    matrix->N = N;
    matrix->nz = nz;

    // Allocate CSR arrays
    matrix->row_pnt = (int*) calloc((M + 1), sizeof(int)); //calloc to initialize to 0
    matrix->col_pnt = (int*) malloc(nz * sizeof(int));
    matrix->val_pnt = (double*) malloc(nz * sizeof(double));
    if (matrix->row_pnt == NULL || matrix->col_pnt == NULL || matrix->val_pnt == NULL) {
        printf("Error: malloc failed for CSR arrays\n");
        free(triplets);
        return 1;
    }

    // Count the number of entries in each row
    for (i = 0; i < nz; i++) {
        matrix->row_pnt[triplets[i].row + 1]++;
    }

    // Create the row pointers (prefix sum)
    for(i = 0; i < M; i++){
        matrix->row_pnt[i+1] += matrix->row_pnt[i];
    }

    // Distribute the data
    int* row_pos = (int*) malloc((M + 1) * sizeof(int));
    if (row_pos == NULL) {
        printf("Error: malloc failed for row_pos\n");
        free(triplets);
        return 1;
    }
    memcpy(row_pos, matrix->row_pnt, (M + 1) * sizeof(int));

    // Loop through the unsorted triplets one more time
    for (i = 0; i < nz; i++) {
        int row = triplets[i].row;
        int col = triplets[i].col;
        double val = triplets[i].val;

        // Find the next available slot for this triplet's row
        int dest_idx = row_pos[row];

        // Place the data in that slot
        matrix->col_pnt[dest_idx] = col;
        matrix->val_pnt[dest_idx] = val;

        // Increment the slot-tracker for that row
        row_pos[row]++;
    }
    
    // Free array
    free(triplets);
    free(row_pos);

    printf("Matrix loaded: %d x %d with %d non-zeros\n", M, N, nz);

    return 0;
}

void matrix_free(SparseMatrixCSR* matrix) {
    free(matrix->row_pnt);
    free(matrix->col_pnt);
    free(matrix->val_pnt);
}

void matrix_vector_mul_sequential(const SparseMatrixCSR* matrix, const double* v, double* y) {
    int i, j;
    for (i = 0; i < matrix->M; i++) {
        int start_index = matrix->row_pnt[i];
        int end_index = matrix->row_pnt[i + 1];

        for (j = start_index; j < end_index; j++) {
            y[i] += matrix->val_pnt[j] * v[matrix->col_pnt[j]];
        }
    }
}

void matrix_vector_mul_parallel(const SparseMatrixCSR* matrix, const double* v, double* y){
    int i, j;
    #pragma omp parallel for default(none) shared(matrix, v, y) private(i, j)
    for (i = 0; i < matrix->M; i++) {
        int start_index = matrix->row_pnt[i];
        int end_index = matrix->row_pnt[i + 1];

        for (j = start_index; j < end_index; j++) {
            y[i] += matrix->val_pnt[j] * v[matrix->col_pnt[j]];
        }
    }
}
