#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include "matrix.h"
#include "timer.h"
#include <mpi.h>
#include <time.h>

int main(int argc, char *argv[]){
    double start, end;
    double time_ms;

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
    // my_N = N_global -> not needed
    int my_nz;
    int *my_row_len = NULL; // local row lenght
    int *my_cols = NULL; // local column indices
    double *my_vals = NULL; // local nnz values

    int M_global;
    int N_global;

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
    // MPI_Bcast( void* buffer , MPI_Count count , MPI_Datatype datatype , int root , MPI_Comm comm);
    MPI_Bcast(&M_global, 1, MPI_INT, 0, MPI_COMM_WORLD); 
    MPI_Bcast(&N_global, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank == 0){   
        
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
        for(int row = 0; row < matrix.M; ++row){
            owner = row % size;
            count_rows[owner]++;
            count_nz[owner] += (matrix.row_pnt[row+1] - matrix.row_pnt[row]);
        }
        
        for(int dest=0; dest < size; ++dest){
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
        if (my_row_len == NULL || my_cols == NULL || my_vals == NULL) {
            fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
            fprintf(stderr, "my_row_len, my_cols, my_vals\n"); //debugging
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        // Receive buff
        MPI_Recv(my_row_len, my_M, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(my_cols, my_nz, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(my_vals, my_nz, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
    }

    // Local SpMV

    // we have the row len, we need row pointers:
    int *my_row_pnt = (int*)malloc((my_M+1) * sizeof(int));
    if (my_row_pnt == NULL) {
        fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
        fprintf(stderr, "my_row_pnt\n"); //debugging
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    my_row_pnt[0] = 0;
    for (int i = 0; i < my_M; i++) {
        my_row_pnt[i+1] = my_row_pnt[i] + my_row_len[i];
    }

    double *v = (double*)malloc(N_global * sizeof(double)); // random dense vector
    double *y = (double*)calloc(my_M, sizeof(double)); // result vector

     if (v == NULL || y == NULL) {
        fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
        fprintf(stderr, "v, y\n"); //debugging
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    if (rank == 0){ 
        srand((unsigned int)time(NULL));
        for(int i = 0; i < N_global; ++i){
            v[i] = (double) (rand() % 10);
            //v[i] = 1.0; // for debugging
        }
    }

    // 0 broadcasts the random vector
    MPI_Bcast(v, N_global, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Multiplication
    SparseMatrixCSR local_matrix;
    matrix_init(&local_matrix, my_M, N_global, my_nz, my_row_pnt, my_cols, my_vals);

    MPI_Barrier(MPI_COMM_WORLD); // sync before counting time

    GET_TIME(start);
    // SpMV implementation from D1
    matrix_vector_mul_sequential(&local_matrix, v, y);

    MPI_Barrier(MPI_COMM_WORLD);

    GET_TIME(end);
    time_ms = (end - start) * 1000.0; //milliseconds

    /*
    if(rank==0){
        printf("Rank %d Row 0 Sum: %f\n", rank, y[0]);
        printf("Time: %f\n", time_ms);
    }
    */ //debugging

    // Calculate the max time among all processes
    //MPI_Reduce( const void* sendbuf , void* recvbuf , MPI_Count count , MPI_Datatype datatype , MPI_Op op , int root , MPI_Comm comm);

    struct { 
        double val; 
        int rank; 
    } local_data, max_data;

    local_data.val = time_ms;
    local_data.rank = rank;

    // MPI_DOUBLE_INT requires a struct with val and rank
    // MPI_MAXLOCK finds the max and the rank id of the max
    MPI_Reduce(&local_data, &max_data, 1, MPI_DOUBLE_INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);

    printf("Max_Time: %f (Rank: %d )\n", max_data.val, max_data.rank);

    free(my_row_len);
    free(my_cols);
    free(my_vals);
    free(my_row_pnt);
    free(v);
    free(y);
    MPI_Finalize();
    return 0;
}