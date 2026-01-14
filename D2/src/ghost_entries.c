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

void ghost_exchange_values(GhostPattern *gp, const double *x_local, double *x_ghost_out, MPI_Comm comm) {
    int size = gp->size;
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
    
    MPI_Alltoallv(send_buf, gp->recv_count, send_displs, MPI_DOUBLE, x_ghost_out, gp->need_count, recv_displs, MPI_DOUBLE, comm);
    
    free(send_buf);
    free(send_displs);
    free(recv_displs);
}

}

void ghost_build_extended_vector(const double *x_local, double **x_extended, GhostPattern *gp, int n_local, MPI_Comm comm){
    int total_ghosts = ghost_get_total_ghosts(gp);
    
    // Allocate: [local values | ghost values]
    *x_extended = (double*)malloc((n_local + total_ghosts) * sizeof(double));
    
    // Copy local values
    memcpy(*x_extended, x_local, n_local * sizeof(double));
    
    // Exchange ghost values - place after local values
    ghost_exchange_values(gp, x_local, *x_extended + n_local, comm);
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

/*
void ghost_local_SpMV_old(int my_M, int *my_row_pnt, int *my_cols, double *my_vals, GhostPattern *gp, double *v_local, double *y) {
    for (int row = 0; row < my_M; row++) {
        double sum = 0.0;
        int start_idx = my_row_pnt[row];
        int end_idx = my_row_pnt[row + 1];
        for (int k = start_idx; k < end_idx; k++) {
            int col = my_cols[k];
            double xval = ghost_get_x(gp, v_local, col);
            sum += my_vals[k] * xval;
        }
        y[row] = sum;
    }
}*/

void ghost_local_SpMV(const SparseMatrixCSR *matrix, const double *x_extended, double *y_local, int n_local) {

    for (int row = 0; row < n_local; row++) {

        double sum = 0.0;
        int start = matrix->row_pnt[row];
        int end = matrix->row_pnt[row + 1];
        
        for (int idx = start; idx < end; idx++) {
            int local_col = matrix->col_pnt[idx];  // already renumbered
            sum += matrix->val_pnt[idx] * x_extended[local_col];
        }
        y_local[row] = sum;

    }
}

void ghost_renumber_columns(SparseMatrixCSR *matrix, GhostPattern *gp, int N_global, int n_local){
    int rank = gp->rank;
    int size = gp->size;
    
    // Create mapping: global_col -> local_index
    int *global_to_local = (int*)malloc(N_global * sizeof(int));
    for (int i = 0; i < N_global; i++) {
        global_to_local[i] = -1;
    }
    
    // Map owned columns [0, n_local)
    for (int local_col = 0; local_col < n_local; local_col++) {
        int global_col = local_col * size + rank;  // Your cyclic distribution
        global_to_local[global_col] = local_col;
    }
    
    // Map ghost columns [n_local, n_local + total_ghosts)
    int ghost_offset = n_local;
    for (int p = 0; p < size; p++) {
        for (int i = 0; i < gp->need_count[p]; i++) {
            int global_col = gp->need_idx_from[p][i];
            global_to_local[global_col] = ghost_offset;
            ghost_offset++;
        }
    }
    
    // Renumber all column indices
    for (int idx = 0; idx < matrix->nz; idx++) {
        int global_col = matrix->col_pnt[idx];
        int local_col = global_to_local[global_col];
        
        if (local_col == -1) {
            fprintf(stderr, "Error: column %d not mapped!\n", global_col);
        }
        matrix->col_pnt[idx] = local_col;
    }
    
    free(global_to_local);
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
