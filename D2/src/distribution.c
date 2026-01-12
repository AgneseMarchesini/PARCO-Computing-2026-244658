#include "distribution.h"
#include "mmio.h"
#include <stdlib.h>
#include <stdio.h>

int distribute_matrix_cyclic(const char *matrix_file, LocalMatrix *local, int rank, int size, MPI_Comm comm) {
    SparseMatrixCSR matrix;
    MPI_Status status;
    
    // Rank 0 loads and broadcasts dimensions
    if (rank == 0) {
        if (matrix_load_from_mtx(matrix_file, &matrix) != 0) {
            fprintf(stderr, "ERROR: Failed to load matrix: %s\n", matrix_file);
            MPI_Abort(comm, 1);
        }
        local->M_global = matrix.M;
        local->N_global = matrix.N;
        local->NZ_global = matrix.nz;
    }
    
    // Broadcasting the matrix dimensions
    // MPI_Bcast( void* buffer , MPI_Count count , MPI_Datatype datatype , int root , MPI_Comm comm);
    MPI_Bcast(&local->M_global, 1, MPI_INT, 0, comm);
    MPI_Bcast(&local->N_global, 1, MPI_INT, 0, comm);
    MPI_Bcast(&local->NZ_global, 1, MPI_INT, 0, comm);
    
    // Distribution logic
    if (rank == 0) {
        // Calculating how big the nnz and rows for the buffer
        int *count_nz = (int*)calloc(size, sizeof(int));
        int *count_rows = (int*)calloc(size, sizeof(int));
        if (count_nz == NULL || count_rows == NULL) {
            fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
            fprintf(stderr, "count_rows, count_nz\n"); //debugging
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        int owner;
        for (int row = 0; row < matrix.M; ++row) {
            owner = row % size;
            count_rows[owner]++;
            count_nz[owner] += (matrix.row_pnt[row+1] - matrix.row_pnt[row]);
        }
        
        // Pack and send to each rank
        for (int dest = 0; dest < size; ++dest) {
            int dest_nz = count_nz[dest];
            int dest_rows = count_rows[dest];
            
            // Buffers
            int *buf_row_len = (int*)malloc(dest_rows * sizeof(int));
            int *buf_cols = (int*)malloc(dest_nz * sizeof(int));
            double *buf_vals = (double*)malloc(dest_nz * sizeof(double));
            if (buf_row_len == NULL || buf_cols == NULL || buf_vals == NULL) {
                fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
                fprintf(stderr, "buf_row_len, buf_cols, buf_vals\n"); //debugging
                MPI_Abort(MPI_COMM_WORLD, 1);
                return 1;
            }

            int current_index = 0;
            int row_index = 0;
            for (int row = dest; row < matrix.M; row += size) {
                buf_row_len[row_index++] = matrix.row_pnt[row+1] - matrix.row_pnt[row]; // store row lenght

                for (int i = matrix.row_pnt[row]; i < matrix.row_pnt[row+1]; i++) {
                    buf_cols[current_index] = matrix.col_pnt[i];
                    buf_vals[current_index] = matrix.val_pnt[i];
                    current_index++;
                }
            }
            
            if (dest == 0) {
                local->my_M = dest_rows;
                local->my_nz = dest_nz;
                local->my_row_len = buf_row_len;
                local->my_cols = buf_cols;
                local->my_vals = buf_vals;
            } else {
                // MPI_Send( const void* buf , MPI_Count count , MPI_Datatype datatype , int dest , int tag , MPI_Comm comm);
                MPI_Send(&dest_rows, 1, MPI_INT, dest, 0, comm);
                MPI_Send(&dest_nz, 1, MPI_INT, dest, 0, comm);

                // Send buff
                MPI_Send(buf_row_len, dest_rows, MPI_INT, dest, 0, comm);
                MPI_Send(buf_cols, dest_nz, MPI_INT, dest, 0, comm);
                MPI_Send(buf_vals, dest_nz, MPI_DOUBLE, dest, 0, comm);
                
                free(buf_row_len);
                free(buf_cols);
                free(buf_vals);
            }
        }
        
        free(count_rows);
        free(count_nz);
        matrix_free(&matrix);
    } else {
        //MPI_Recv( void* buf , MPI_Count count , MPI_Datatype datatype , int source , int tag , MPI_Comm comm , MPI_Status* status);
        MPI_Recv(&local->my_M, 1, MPI_INT, 0, 0, comm, &status);
        MPI_Recv(&local->my_nz, 1, MPI_INT, 0, 0, comm, &status);
        
        local->my_row_len = (int*)malloc(local->my_M * sizeof(int));
        local->my_cols = (int*)malloc(local->my_nz * sizeof(int));
        local->my_vals = (double*)malloc(local->my_nz * sizeof(double));
        if (my_row_len == NULL || my_cols == NULL || my_vals == NULL) {
            fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
            fprintf(stderr, "my_row_len, my_cols, my_vals\n"); //debugging
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
        
        // Receive buff
        MPI_Recv(local->my_row_len, local->my_M, MPI_INT, 0, 0, comm, &status);
        MPI_Recv(local->my_cols, local->my_nz, MPI_INT, 0, 0, comm, &status);
        MPI_Recv(local->my_vals, local->my_nz, MPI_DOUBLE, 0, 0, comm, &status);
    }
    
    
    // We have the row len, we need row pointers:
    local->my_row_pnt = (int*)malloc((local->my_M + 1) * sizeof(int));
    if (my_row_pnt == NULL) {
        fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
        fprintf(stderr, "my_row_pnt\n"); //debugging
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    local->my_row_pnt[0] = 0;
    for (int i = 0; i < local->my_M; i++) {
        local->my_row_pnt[i+1] = local->my_row_pnt[i] + local->my_row_len[i];
    }
    
    return 0;
}

void local_matrix_free(LocalMatrix *local) {
    free(local->my_row_len);
    free(local->my_row_pnt);
    free(local->my_cols);
    free(local->my_vals);
}
