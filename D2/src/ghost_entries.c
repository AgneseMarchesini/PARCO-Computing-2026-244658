#include <stdlib.h>
#include <string.h>
#include "ghost_entries.h"

static int owner_of(const GhostPattern *gp, int idx) {
    return idx % gp->size;
}

void ghost_build_ownership(GhostPattern *gp, int size, int rank, const int *recvcounts){
    gp->size = size;
    gp->rank = rank;

    gp->row_start = (int*) malloc(size * sizeof(int));
    gp->row_end   = (int*) malloc(size * sizeof(int));
    
    for (int r = 0; r < size; r++) {
        gp->row_start[r] = 0;
        gp->row_end[r]   = 0;
    }
}


void ghost_build_pattern(GhostPattern *gp, const SparseMatrixCSR *matrix){
    int size = gp->size;
    int rank = gp->rank;

    gp->need_idx_from = (int**) calloc(size, sizeof(int*));
    gp->need_count    = (int*)  calloc(size, sizeof(int));
    int *capacity     = (int*)  calloc(size, sizeof(int));

    for (int p = 0; p < size; p++) {
        capacity[p] = 16;
        gp->need_idx_from[p] = (int*) malloc(capacity[p] * sizeof(int));
    }

    for (int row = 0; row < matrix->M; row++) {
        int start = matrix->row_pnt[row];
        int end   = matrix->row_pnt[row + 1];
        for (int k = start; k < end; k++) {
            int col = matrix->col_pnt[k];
            int owner = owner_of(gp, col);
            if (owner == rank || owner < 0) continue;

            int c = gp->need_count[owner];
            if (c == capacity[owner]) {
                capacity[owner] *= 2;
                gp->need_idx_from[owner] =
                    (int*) realloc(gp->need_idx_from[owner],
                                   capacity[owner] * sizeof(int));
            }
            gp->need_idx_from[owner][c] = col;
            gp->need_count[owner]++;
        }
    }

    free(capacity);
}

void ghost_exchange_index_lists(GhostPattern *gp, MPI_Comm comm){
    int size = gp->size;
    int rank = gp->rank;

    gp->recv_count = (int*) calloc(size, sizeof(int));
    MPI_Alltoall(gp->need_count, 1, MPI_INT, gp->recv_count, 1, MPI_INT, comm);

    // Compute send/recv displacements for Alltoallv
    int *send_displs = (int*)malloc(size * sizeof(int));
    int *recv_displs = (int*)malloc(size * sizeof(int));
    send_displs[0] = 0;
    recv_displs[0] = 0;
    for (int p = 1; p < size; p++) {
        send_displs[p] = send_displs[p-1] + gp->need_count[p-1];
        recv_displs[p] = recv_displs[p-1] + gp->recv_count[p-1];
    }

    // Pack all send data contiguously
    int total_send = send_displs[size-1] + gp->need_count[size-1];
    int total_recv = recv_displs[size-1] + gp->recv_count[size-1];
    int *send_buf = (int*)malloc(total_send * sizeof(int));
    int *recv_buf = (int*)malloc(total_recv * sizeof(int));

    for (int p = 0; p < size; p++) {
        memcpy(send_buf + send_displs[p], gp->need_idx_from[p], gp->need_count[p] * sizeof(int));
    }

    // Single Alltoallv exchange
    MPI_Alltoallv(send_buf, gp->need_count, send_displs, MPI_INT, recv_buf, gp->recv_count, recv_displs, MPI_INT, comm);

    // Unpack received data
    gp->send_idx_to = (int**) calloc(size, sizeof(int*));
    for (int p = 0; p < size; p++) {
        if (gp->recv_count[p] > 0) {
            gp->send_idx_to[p] = (int*) malloc(gp->recv_count[p] * sizeof(int));
            memcpy(gp->send_idx_to[p], recv_buf + recv_displs[p], gp->recv_count[p] * sizeof(int));
        }
    }

    free(send_buf);
    free(recv_buf);
    free(send_displs);
    free(recv_displs);

    gp->ghost_disp = (int*) malloc(size * sizeof(int));
    int total_ghosts = 0;
    for (int p = 0; p < size; p++) {
        gp->ghost_disp[p] = total_ghosts;
        total_ghosts += gp->need_count[p];
    }
    gp->ghost_x = (double*) malloc(total_ghosts * sizeof(double));
}

void ghost_exchange_values(GhostPattern *gp, const double *x_local, MPI_Comm comm){
    int size = gp->size;
    int rank = gp->rank;

    // Compute displacements
    int *send_displs = (int*)malloc(size * sizeof(int));
    int *recv_displs = (int*)malloc(size * sizeof(int));
    send_displs[0] = 0;
    recv_displs[0] = 0;
    for (int p = 1; p < size; p++) {
        send_displs[p] = send_displs[p-1] + gp->recv_count[p-1];
        recv_displs[p] = recv_displs[p-1] + gp->need_count[p-1];
    }

    // Pack send data
    int total_send = send_displs[size-1] + gp->recv_count[size-1];
    double *send_buf = (double*)malloc(total_send * sizeof(double));

    for (int p = 0; p < size; p++) {
        for (int i = 0; i < gp->recv_count[p]; i++) {
            int global_idx = gp->send_idx_to[p][i];
            int local_idx = global_idx / size;
            send_buf[send_displs[p] + i] = x_local[local_idx];
        }
    }

    // Single Alltoallv exchange
    MPI_Alltoallv(send_buf, gp->recv_count, send_displs, MPI_DOUBLE, gp->ghost_x, gp->need_count, gp->ghost_disp, MPI_DOUBLE, comm);

    free(send_buf);
    free(send_displs);
    free(recv_displs);
}

double ghost_get_x(const GhostPattern *gp, const double *x_local, int col){
    int owner = col % gp->size;  
    
    if (owner == gp->rank) {
        int local_idx = col / gp->size;
        return x_local[local_idx];
    }

    int base = gp->ghost_disp[owner];
    int len  = gp->need_count[owner];
    for (int i = 0; i < len; i++) {
        if (gp->need_idx_from[owner][i] == col) {
            return gp->ghost_x[base + i];
        }
    }
    return 0.0;
}


void ghost_free(GhostPattern *gp){
    if (!gp) return;

    for (int p = 0; p < gp->size; p++) {
        free(gp->need_idx_from ? gp->need_idx_from[p] : NULL);
        free(gp->send_idx_to  ? gp->send_idx_to[p]   : NULL);
    }

    free(gp->need_idx_from);
    free(gp->send_idx_to);
    free(gp->need_count);
    free(gp->recv_count);
    free(gp->row_start);
    free(gp->row_end);
    free(gp->ghost_disp);
    free(gp->ghost_x);
}
