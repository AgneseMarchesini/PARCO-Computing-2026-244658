#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include "matrix.h"
#include <mpi.h>

typedef struct {
    int my_M;           // local number of rows
    int my_nz;          // local number of non-zeros
    int *my_row_len;    // local row lengths
    int *my_row_pnt;    // local row pointers (CSR format)
    int *my_cols;       // local column indices
    double *my_vals;    // local values
    int M_global;       // global matrix dimensions
    int N_global;
    int NZ_global;
} LocalMatrix;

// Load matrix and distribute to all ranks using cyclic row distribution
int distribute_matrix_cyclic(const char *matrix_file, LocalMatrix *local, int rank, int size, MPI_Comm comm);

// Free local matrix resourcess
void local_matrix_free(LocalMatrix *local);

#endif
