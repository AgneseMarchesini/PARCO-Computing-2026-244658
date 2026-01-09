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

void ghost_exchange_index_lists(GhostPattern *gp, int N_global, MPI_Comm comm){
    int size = gp->size;
    int rank = gp->rank;
    
    gp->recv_count = (int*) calloc(size, sizeof(int));
    MPI_Alltoall(gp->need_count, 1, MPI_INT, gp->recv_count, 1, MPI_INT, comm);

    int *send_displs = (int*)malloc(size * sizeof(int));
    send_displs[0] = 0;
    for (int p = 1; p < size; p++) {
        send_displs[p] = send_displs[p-1] + gp->need_count[p-1];
    }
    int total_send = send_displs[size-1] + gp->need_count[size-1];
    int *send_buf = (int*)malloc(total_send * sizeof(int));
    for (int p = 0; p < size; p++) {
        memcpy(send_buf + send_displs[p], gp->need_idx_from[p], gp->need_count[p] * sizeof(int));
    }

    gp->send_idx_to = (int**) calloc(size, sizeof(int*));
    for (int p = 0; p < size; p++) {
        if (gp->recv_count[p] > 0) {
            gp->send_idx_to[p] = (int*)malloc(gp->recv_count[p] * sizeof(int));
            
            if (p == rank) {
                // Self-copy
                memcpy(gp->send_idx_to[p], send_buf + send_displs[p], gp->recv_count[p] * sizeof(int));
            } else {
                // Receive from p (their send to us)
                MPI_Recv(gp->send_idx_to[p], gp->recv_count[p], MPI_INT, p, 0, comm, MPI_STATUS_IGNORE);
            }
        }
        
        // Send to p (if needed)
        if (p != rank && gp->need_count[p] > 0) {
            MPI_Send(gp->need_idx_from[p], gp->need_count[p], MPI_INT, p, 0, comm);
        }
    }

    free(send_buf);
    free(send_displs);

    gp->ghost_disp = (int*) malloc(size * sizeof(int));
    int total_ghosts = 0;
    for (int p = 0; p < size; p++) {
        gp->ghost_disp[p] = total_ghosts;
        total_ghosts += gp->need_count[p];
    }
    gp->ghost_x = (double*) malloc(total_ghosts * sizeof(double));

    gp->col_to_ghost = (int*) malloc(N_global * sizeof(int));
    for (int i = 0; i < N_global; i++) {
        gp->col_to_ghost[i] = -1; 
    }

    for (int p = 0; p < size; p++) {
        for (int i = 0; i < gp->need_count[p]; i++) {
            int col = gp->need_idx_from[p][i];
            gp->col_to_ghost[col] = gp->ghost_disp[p] + i;
        }
    }
}

void ghost_exchange_values(GhostPattern *gp, const double *x_local, MPI_Comm comm){
    int size = gp->size;
    int rank = gp->rank;

    int *send_displs = (int*)malloc(size * sizeof(int));
    send_displs[0] = 0;
    for (int p = 1; p < size; p++) {
        send_displs[p] = send_displs[p-1] + gp->recv_count[p-1];
    }
    int total_send = send_displs[size-1] + gp->recv_count[size-1];
    double *send_buf = (double*)malloc(total_send * sizeof(double));
    for (int p = 0; p < size; p++) {
        for (int i = 0; i < gp->recv_count[p]; i++) {
            int global_idx = gp->send_idx_to[p][i];
            int local_idx = global_idx / size;
            send_buf[send_displs[p] + i] = x_local[local_idx];
        }
    }

    for (int p = 0; p < size; p++) {
        if (gp->need_count[p] > 0) {
            if (p == rank) {
                // Self-copy local values to ghost_x
                for (int i = 0; i < gp->need_count[p]; i++) {
                    int col = gp->need_idx_from[p][i];
                    gp->ghost_x[gp->ghost_disp[p] + i] = x_local[col / size];
                }
            } else {
                // Receive ghosts from p
                MPI_Recv(gp->ghost_x + gp->ghost_disp[p], gp->need_count[p], MPI_DOUBLE, p, 1, comm, MPI_STATUS_IGNORE);
                // Send values to p
                MPI_Send(send_buf + send_displs[p], gp->recv_count[p], MPI_DOUBLE, p, 1, comm);
            }
        }
    }

    free(send_buf);
    free(send_displs);
}


double ghost_get_x(const GhostPattern *gp, const double *x_local, int col){
    int owner = col % gp->size;
    
    if (owner == gp->rank) {
        return x_local[col / gp->size];
    }
    
    int ghost_pos = gp->col_to_ghost[col];
    return gp->ghost_x[ghost_pos];
}

int ghost_get_total_ghosts(const GhostPattern *gp) {
    int total = 0;
    for (int p = 0; p < gp->size; p++) {
        total += gp->need_count[p];
    }
    return total;
}


void ghost_free(GhostPattern *gp){
    if (!gp) return;

    for (int p = 0; p < gp->size; p++) {
        free(gp->need_idx_from ? gp->need_idx_from[p] : NULL);
        free(gp->send_idx_to  ? gp->send_idx_to[p]   : NULL);
    }

    free(gp->col_to_ghost);
    free(gp->need_idx_from);
    free(gp->send_idx_to);
    free(gp->need_count);
    free(gp->recv_count);
    free(gp->row_start);
    free(gp->row_end);
    free(gp->ghost_disp);
    free(gp->ghost_x);
}
