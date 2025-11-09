#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mmio.h"
#include "matrix.h"

// Temporary container for (row, column, value) triplet
typedef struct {
    int row;
    int col;
    double val;
} Triplet;

// Compare function
static int compare_triplets(const void* a, const void* b) {
    Triplet* ta = (Triplet*)a;
    Triplet* tb = (Triplet*)b;
    if (ta->row < tb->row) return -1;
    if (ta->row > tb->row) return 1; 
    if (ta->col < tb->col) return -1;
    if (ta->col > tb->col) return 1;
    return 0;
}

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

    // Sort triplets by row and then by column
    qsort(triplets, nz, sizeof(Triplet), compare_triplets);

    // Popolate matrix
    matrix->M = M;
    matrix->N = N;
    matrix->nz = nz;

    // Allocate CSR arrays
    matrix->row_pnt = (int*) calloc((M + 1), sizeof(int)); //calloc to initialize to 0
    matrix->col_pnt = (int*) malloc(nz * sizeof(int));
    matrix->val_pnt = (double*) malloc(nz * sizeof(double));

    // Fill CSR arrays
    for (i = 0; i < nz; i++) {
        matrix->row_pnt[triplets[i].row + 1]++;
        matrix->col_pnt[i] = triplets[i].col;
        matrix->val_pnt[i] = triplets[i].val;
    }

    // Convert the row counts into cumulative sums
    for(int i = 0; i < M; i++){
        matrix->row_pnt[i+1] += matrix->row_pnt[i]; //current position = previous position + number of nz
    }
    
    // Free temporary triplet array
    free(triplets);

    printf("Matrix loaded: %d x %d with %d non-zeros\n", M, N, nz);

    return 0;
}

void matrix_free(SparseMatrixCSR* matrix) {
    free(matrix->row_pnt);
    free(matrix->col_pnt);
    free(matrix->val_pnt);
}

void matrix_vector_mul(const SparseMatrixCSR* matrix, const double* v, double* y) {
    for (int i = 0; i < matrix->M; i++) {
        y[i] = 0.0;
        int start_index = matrix->row_pnt[i];
        int end_index = matrix->row_pnt[i + 1];

        for (int j = start_index; j < end_index; j++) {
            y[i] += matrix->val_pnt[j] * v[matrix->col_pnt[j]];
        }
    }
}