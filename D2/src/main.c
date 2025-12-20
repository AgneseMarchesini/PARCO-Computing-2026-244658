#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include "matrix.h"
#include "timer.h"
#include <mpi.h>
#include <time.h>

int main(int argc, char *argv[]){
    int rank, size;
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0)
            fprintf(stderr, "Run with at least 2 processes.\n");
        MPI_Finalize();
        return 1;
    }

    if(argc != 2){
        printf("Usage: ");
        return 1;
    }

    char* matrix_file = argv[1];
    SparseMatrixCSR matrix;

    // Variables for local partition
    int my_M;
    int my_nz;
    int *my_row_len = NULL; // local row lenght
    int *my_cols = NULL; // local column indices
    int *my_vals = NULL; // local nnz values

    if(rank == 0){
        if (matrix_load_from_mtx(matrix_file, &matrix) != 0) {
            fprintf(stderr, "CRITICAL ERROR: Failed to load matrix: %s\n", matrix_file);
            fprintf(stderr, "Check if the 'data' directory and .mtx file exist.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        M_global = matrix.M;
        N_global = matrix.N;
    }

    // Broadcasting the matrix dimensions
    MPI_Bcast(&M_global, 1, MPI_INT, 0, MPI_COMM_WORLD); 
    MPI_Bcast(&N_global, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank == 0){   
        // Initialize random dense vector
        srand((unsigned int)time(NULL));
        for(i = 0; i < matrix.N; i++) {
            v[i] = (double)(rand() % 10);
        }


        // Calculating how big the nnz and rows for the buffer
        int *count_nz = (int*)calloc(size, sizeof(int));
        int *count_rows = (int*)calloc(size, sizeof(int));
        int owner;
        for(int row = 0; row < matrix.M; ++row){
            owner = row % size;
            count_rows[owner]++
            count_nz[owner] += (matrix.row_pnt[row+1] - matrix.row_pnt[i]);
        }
        
        for(int dest=0; dest < size; ++dest){
            int dest_nz = count_nz[dest];
            int dest_rows = count_rows[dest];

            // Buffers
            int *buf_row_len = (int*)malloc(dest_rows * sizeof(int));
            int *buf_cols = (int*)malloc(dest_nz * sizeof(int));
            double *buf_vals = (double*)malloc(dest_nz * sizeof(double));

            //
            int current_index = 0;
            int row_index = 0;
            for (int row = dest; row < matrix.M; row += size){
                buf_row_len[row_index++] = (matrix.row_pnt[row+1] - matrix.row_pnt[row]); // store row lenght

                for (int i = matrix.row_pnt[row]; i < matrix.row_pnt[row+1]; i++){
                    buf_cols[current_index] = matrix.col_pnt[i];
                    buf_vals[current_index] = matrix.val_pnt[i];
                    current_index++;
                }
            }

            if (dest == 0){
                my_M = dest_rows;
                my_nz = dest_nz;
                my_row_len = buf_row_len;
                my_cols = buf_cols;
                my_vals = buf_vals;
            } else{
                // MPI_Send( const void* buf , MPI_Count count , MPI_Datatype datatype , int dest , int tag , MPI_Comm comm);
                MPI_Send(&dest_rows, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
                MPI_Send(&dest_nz, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);

                // Send buff
                MPI_Send(buf_row_len, dest_rows, MPI_INT, dest, 0, MPI_COMM_WORLD);
                MPI_Send(buf_cols, dest_nz, MPI_INT, dest, 0, MPI_COMM_WORLD);
                MPI_Send(buf_vals, dest_nz, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD);

                free(buf_row_len);
                free(buf_cols);
                free(buf_vals);
            }

        }

        free(count_rows);
        free(count_nz);
        matrix_free(&matrix);
    }
    else {
        //MPI_Recv( void* buf , MPI_Count count , MPI_Datatype datatype , int source , int tag , MPI_Comm comm , MPI_Status* status);
        MPI_Recv(&my_M, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&my_nz, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        my_row_len = (int*)malloc(my_M * sizeof(int));
        my_cols = (int*)malloc(my_nz* sizeof(int));
        my_vals = (double*)malloc(my_nz* sizeof(double));

        // Receive buff
        MPI_Recv(my_row_len, my_M, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(my_cols, my_nz, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(my_vals, my_nz, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
    }

    /*
    double* v = (double*)malloc(matrix.N * sizeof(double)); // dense random vector
        double* y = (double*)calloc(matrix.M, sizeof(double)); // result vector
        if (v == NULL || y == NULL) {
            fprintf(stderr, "CRITICAL ERROR: Malloc failed. (Matrix M/N: %d, %d)\n", matrix.M, matrix.N);
            return 1;
        }
    */

    free(my_row_len);
    free(my_cols);
    free(my_vals);
    MPI_Finalize();
    return 0;
}