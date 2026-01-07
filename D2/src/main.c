#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mmio.h"
#include "matrix.h"
#include "ghost_entries.h"
#include "distvec.h"
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

    if(argc != 3){
        if (rank == 0) {
            printf("Usage: %s <matrix_file> <mode>\n", argv[0]);
            printf("Mode 0 = replicated vector\n");
            printf("Mode 1 = ghost entries\n");
        }
        MPI_Finalize();
        return 1;
    }

    char* matrix_file = argv[1];
    int mode = atoi(argv[2]); // 0 or 1
    if(mode != 0 && mode != 1){
        if(rank == 0){
            fprintf(stderr, "ERROR: mode must be 0 or 1 \n");
        }
        MPI_Finalize();
        return 1;
    }

    bool use_ghost;
    if(mode == 1){
        use_ghost = true;
    } else use_ghost = false;

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
    dist_build_counts_displs(size, my_M, recvcounts_vec, displs_vec, MPI_COMM_WORLD);

    double *v_local = (double*)malloc(my_M * sizeof(double)); // random dense vector
    double *y = (double*)calloc(my_M, sizeof(double)); // result vector
    /* for(int i = 0; i < my_M; ++i){
        y[i]=1.0;
    }
        */

    if (v_local == NULL || y == NULL) {
        fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
        fprintf(stderr, "v_local, y\n"); //debugging
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    // Initialize LOCAL chunk of v randomly on each rank
    srand((unsigned int)(time(NULL) + rank * 12345));  // different seed per rank
    for (int i = 0; i < my_M; i++) {
        v_local[i] = (double)(rand() % 10);
    }


    // Multiplication
    SparseMatrixCSR local_matrix;
    matrix_init(&local_matrix, my_M, N_global, my_nz, my_row_pnt, my_cols, my_vals);

    // Build ghost pattern 
    GhostPattern gp;
    if(use_ghost){
        ghost_build_ownership(&gp, size, rank, recvcounts_vec);
        ghost_build_pattern(&gp, &local_matrix);
        ghost_exchange_index_lists(&gp, N_global, MPI_COMM_WORLD);
    }
    
    MPI_Barrier(MPI_COMM_WORLD); // sync before counting time

    int iterations = 10;

    for(int i=0; i<iterations; i++){

        //debug
        if(rank == 0) printf("\n=== Iteration %d START ===\n", i); fflush(stdout);

        GET_TIME(start);
        if(!use_ghost){ 
            double *v_full = (double*)malloc(N_global * sizeof(double));
            if (!v_full) { 
                fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
                fprintf(stderr, "v_full\n"); //debugging
                MPI_Abort(MPI_COMM_WORLD, 1);
                return 1;
            }
            MPI_Allgatherv(v_local, my_M, MPI_DOUBLE, v_full, recvcounts_vec, displs_vec, MPI_DOUBLE, MPI_COMM_WORLD);
            GET_TIME(end);
            time_comm += (end - start);

            // Computation phase
            GET_TIME(start);
            matrix_vector_mul_sequential(&local_matrix, v_full, y);
            GET_TIME(end);
            time_comp += (end - start);
            free(v_full);
        } else {
            //debug
            printf("[Rank %d] Before ghost_exchange_values\n", rank); fflush(stdout);
            ghost_exchange_values(&gp, v_local, MPI_COMM_WORLD);
            //debug
            printf("[Rank %d] After ghost_exchange_values\n", rank); fflush(stdout);
            GET_TIME(end);
            time_comm += (end - start);

            GET_TIME(start);
            for (int row = 0; row < my_M; row++) {

                //debug
                //printf("[Rank %d] Before inline SpMV\n", rank); fflush(stdout);
                double sum = 0.0;
                int start_idx = my_row_pnt[row];
                int end_idx   = my_row_pnt[row + 1];
                for (int k = start_idx; k < end_idx; k++) {
                    int col = my_cols[k];
                    double xval = ghost_get_x(&gp, v_local, col);
                    sum += my_vals[k] * xval;
                }
                y[row] = sum;
                //debug
                //printf("[Rank %d] After inline SpMV\n", rank); fflush(stdout);
            }
            GET_TIME(end);
            time_comp += (end - start);
        }


        time_comp += (end-start);
        //debug
        if(rank == 0) printf("=== Iteration %d END ===\n", i); fflush(stdout);
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
    mem_bytes += (use_ghost ? my_M : N_global) * sizeof(double); // vector x 
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
        printf("Comm_Volume_MB: %f \n", comm_mb_max);
    }

    if(use_ghost){
        ghost_free(&gp);
    }
    free(my_row_len);
    free(my_cols);
    free(my_vals);
    free(my_row_pnt);
    free(v_local);
    free(y);
    free(recvcounts_vec);
    free(displs_vec);
    MPI_Finalize();
    return 0;
}