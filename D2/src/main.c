#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include "matrix.h"
#include "timer.h"
#include <mpi.h>
#include <time.h>

int main(int argc, char *argv[]){
    double start, end;
    double time_comm = 0.0;
    double time_comp = 0.0;

    int rank, size;
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /*
    if (size < 2) {
        MPI_Finalize();
        return 1;
    }
    */

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
    int NZ_global;

    if(rank == 0){
        if (matrix_load_from_mtx(matrix_file, &matrix) != 0) {
            fprintf(stderr, "CRITICAL ERROR: Failed to load matrix: %s\n", matrix_file);
            fprintf(stderr, "Check if the 'data' directory and .mtx file exist.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        M_global = matrix.M;
        N_global = matrix.N;
        NZ_global = matrix.nz;
    }

    // Broadcasting the matrix dimensions
    // MPI_Bcast( void* buffer , MPI_Count count , MPI_Datatype datatype , int root , MPI_Comm comm);
    MPI_Bcast(&M_global, 1, MPI_INT, 0, MPI_COMM_WORLD); 
    MPI_Bcast(&N_global, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&NZ_global, 1, MPI_INT, 0, MPI_COMM_WORLD);

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

    int *recvcounts_vec = (int *)malloc(size * sizeof(int));
    int *displs_vec = (int *)malloc(size * sizeof(int));

    // every rank sends its my_M to everyone else
    MPI_Allgather(&my_M, 1, MPI_INT, recvcounts_vec, 1, MPI_INT, MPI_COMM_WORLD);

    // calculate displacements (where each rank's chunk starts in the global vector)
    displs_vec[0] = 0;
    for (int i = 1; i < size; i++) {
        displs_vec[i] = displs_vec[i-1] + recvcounts_vec[i-1];
    }

    double *v = (double*)malloc(N_global * sizeof(double)); // random dense vector
    double *y = (double*)calloc(my_M, sizeof(double)); // result vector
    for(int i = 0; i < my_M; ++i){
        y[i]=1.0;
    }

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

    // Multiplication
    SparseMatrixCSR local_matrix;
    matrix_init(&local_matrix, my_M, N_global, my_nz, my_row_pnt, my_cols, my_vals);

    MPI_Barrier(MPI_COMM_WORLD); // sync before counting time

    int iterations = 10;

    // SpMV implementation from D1
    for(int i=0; i<iterations; i++){

        GET_TIME(start);
        // 0 broadcasts the random vector
        // MPI_Bcast(v, N_global, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        // MPI_Allgatherv( const void* sendbuf , MPI_Count sendcount , MPI_Datatype sendtype , void* recvbuf , const MPI_Count recvcounts[] , const MPI_Aint displs[] , MPI_Datatype recvtype , MPI_Comm comm);
        MPI_Allgatherv(y, my_M, MPI_DOUBLE, v, recvcounts_vec, displs_vec, MPI_DOUBLE, MPI_COMM_WORLD);
        GET_TIME(end);

        time_comm += (end-start);

        GET_TIME(start);
        matrix_vector_mul_sequential(&local_matrix, v, y);
        GET_TIME(end);

        time_comp += (end-start);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double avg_comm = (time_comm * 1000.0) / iterations;
    double avg_comp = (time_comp * 1000.0) / iterations;

    double checksum = 0.0;
    for(int i=0; i<my_M; i++) checksum += y[i];
    if (rank == 0) printf("DEBUG_CHECKSUM: %f\n", checksum);
    
    /*
    if(rank==0){
        printf("Rank %d Row 0 Sum: %f\n", rank, y[0]);
        printf("Time: %f\n", time_ms);
    }
    */ //debugging

    // Calculate the max time among all processes
    //MPI_Reduce( const void* sendbuf , void* recvbuf , MPI_Count count , MPI_Datatype datatype , MPI_Op op , int root , MPI_Comm comm);

    double max_comm = 0.0;
    double max_comp = 0.0;
    MPI_Reduce(&avg_comm, &max_comm, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&avg_comp, &max_comp, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // flops
    double gflops = ( (2.0 * (double)NZ_global) / max_comp) / 1e9;

    // memory footprint
    long mem_bytes = 0;
    mem_bytes += local_matrix.nz * sizeof(double);          // val
    mem_bytes += local_matrix.nz * sizeof(int);             // col
    mem_bytes += (my_M + 1) * sizeof(int);                  // row_ptr
    mem_bytes += N_global * sizeof(double);                 // vector x 
    mem_bytes += my_M * sizeof(double);                     // vector y 

    double mem_mb = mem_bytes / (1024.0 * 1024.0);
    double max_mem_mb = 0.0;

    MPI_Reduce(&mem_mb, &max_mem_mb, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // NNZ per rank (min,max,avg)
    int min_nz, max_nz, sum_nz;
    MPI_Reduce(&my_nz, &min_nz, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&my_nz, &max_nz, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&my_nz, &sum_nz, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Communication volume per rank
    long comm_volume_bytes = (N_global * sizeof(double)) - (my_M * sizeof(double)); // total vector size - my local part
    long max_comm_volume;
    MPI_Reduce(&comm_volume_bytes, &max_comm_volume, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
    double comm_mb_max = max_comm_volume / (1024.0 * 1024.0); //conversion to mb


    if (rank == 0) {
        double avg_nz = (double)sum_nz / size;

        printf("Max_Comm: %f Max_Comp: %f GFLOPS: %f Mem_MB: %f \n", max_comm, max_comp, gflops, max_mem_mb);
        printf("NZ_Min: %d \n", min_nz);
        printf("NZ_Max: %d \n", max_nz);
        printf("NZ_Avg: %f \n", avg_nz);
        printf("Comm_Volume_MB: %f ", comm_mb_max);
    }

    free(my_row_len);
    free(my_cols);
    free(my_vals);
    free(my_row_pnt);
    free(v);
    free(y);
    free(recvcounts_vec);
    free(displs_vec);
    MPI_Finalize();
    return 0;
}