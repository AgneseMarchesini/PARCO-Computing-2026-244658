#ifndef GHOST_ENTRIES_H
#define GHOST_ENTRIES_H

#include <mpi.h>
#include "matrix.h"

// Data needed to handle ghost entries for vector x
typedef struct {
    int  size;
    int  rank;

    int *row_start; // length = size
    int *row_end; // length = size

    // For each neighbor p:
    int **need_idx_from; // indices we need from p
    int  *need_count; // count for each p

    int **send_idx_to; // indices p needs from us
    int  *recv_count; // count for each p

    int  *ghost_disp; // displacement into ghost_x for each p
    double *ghost_x; // concatenated ghost values

    int *col_to_ghost;

} GhostPattern;

// Build ownership ranges (row_start/row_end)
void ghost_build_ownership(GhostPattern *gp, int size, int rank, const int *recvcounts);


// Analyze the local CSR matrix and discover ghost indices.
void ghost_build_pattern(GhostPattern *gp, const SparseMatrixCSR *matrix);


// Exchange index lists with neighbors so that each rank knows which entries of x it must send.
void ghost_exchange_index_lists(GhostPattern *gp, int N_global, MPI_Comm comm);


// For a given input vector x  exchange the ghost values according to the pre-built pattern.
void ghost_exchange_values(GhostPattern *gp, const double *x_local, MPI_Comm comm);


// Get value x[col], either local or ghost, using the pattern.
double ghost_get_x(const GhostPattern *gp, const double *x, int col);


// Free all memory allocated
void ghost_free(GhostPattern *gp);

#endif
