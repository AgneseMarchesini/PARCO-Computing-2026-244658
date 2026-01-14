#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mmio.h"
#include "matrix.h"
#include "ghost_entries.h"
#include "distribution.h"
#include "timer.h"
#include <mpi.h>
#include <time.h>
#include <string.h>

// for qsort
int compare_doubles(const void *a, const void *b) {
    double diff = (*(double*)a - *(double*)b);
    if (diff < 0) return -1;
    if (diff > 0) return 1;
    return 0;
}

// calculate percentile from sorted array 
double percentile(double *sorted_array, int n, double p) {
    double index = p * (n - 1);
    int lower = (int)index;
    int upper = lower + 1;
    
    if (upper >= n) return sorted_array[n-1];
    
    double weight = index - lower;
    return sorted_array[lower] * (1.0 - weight) + sorted_array[upper] * weight;
}

int main(int argc, char *argv[]){
    int rank, size;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(argc != 2){
        if (rank == 0) {
            printf("Usage: %s <matrix_file>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    char* matrix_file = argv[1];

    LocalMatrix local;
    distribute_matrix_cyclic(matrix_file, &local, rank, size, MPI_COMM_WORLD);

    double *v_local = (double*)malloc(local.my_M * sizeof(double)); // random dense vector
    double *y = (double*)calloc(local.my_M, sizeof(double)); // result vector

    if (v_local == NULL || y == NULL) {
        fprintf(stderr, "CRITICAL ERROR: Malloc failed.\n");
        fprintf(stderr, "v_local, y\n"); //debugging
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    // Initialize LOCAL chunk of v randomly on each rank
    //srand(42 + rank*12345); //FIXED SEED TO SEE IF THE DEBUG CHECKSUM IS CORRECT
    srand((unsigned int)(time(NULL) + rank * 12345));  // different seed per rank
    for (int i = 0; i < local.my_M; i++) {
        v_local[i] = (double)(rand() % 10);
    }


    // Multiplication
    SparseMatrixCSR local_matrix;
    matrix_init(&local_matrix, local.my_M, local.N_global, local.my_nz, local.my_row_pnt, local.my_cols, local.my_vals);

    int *recvcounts_vec = (int*)malloc(size * sizeof(int));
    int *displs_vec = (int*)malloc(size * sizeof(int));
    if (recvcounts_vec == NULL || displs_vec == NULL) {
        fprintf(stderr, "CRITICAL ERROR: Malloc failed for recvcounts_vec/displs_vec\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }
    MPI_Allgather(&local.my_M, 1, MPI_INT, recvcounts_vec, 1, MPI_INT, MPI_COMM_WORLD);
    // Compute displs
    displs_vec[0] = 0;
    for (int i = 1; i < size; i++) {
        displs_vec[i] = displs_vec[i-1] + recvcounts_vec[i-1];
    }


    // Build ghost pattern 
    GhostPattern gp;
    ghost_build_ownership(&gp, size, rank, recvcounts_vec);
    ghost_build_pattern(&gp, &local_matrix);
    ghost_exchange_index_lists(&gp, local.N_global, MPI_COMM_WORLD);
    ghost_renumber_columns(&local_matrix, &gp, local.N_global, local.my_M);
    
    MPI_Barrier(MPI_COMM_WORLD); 


    int total_ghosts = ghost_get_total_ghosts(&gp);
    double *x_extended = (double*)malloc((local.my_M + total_ghosts) * sizeof(double));

    // cache warmup
    int warmup = 3;
    for(int i=0; i<warmup; i++){
        ghost_exchange_values(&gp, v_local, x_extended + local.my_M, MPI_COMM_WORLD);
        memcpy(x_extended, v_local, local.my_M * sizeof(double));
        ghost_local_SpMV(&local_matrix, x_extended, y, local.my_M);
    }
    for(int row = 0; row < local.my_M; row++){
        y[row] = 0.0;
    }

    MPI_Barrier(MPI_COMM_WORLD); // sync before counting time

    int iterations = 100;
    double start, end;
    double *times_comm = (double*)malloc(iterations * sizeof(double));
    double *times_comp = (double*)malloc(iterations * sizeof(double));

    for(int i=0; i<iterations; i++){

        GET_TIME(start);
        ghost_exchange_values(&gp, v_local, x_extended + local.my_M, MPI_COMM_WORLD);
        GET_TIME(end);

        times_comm[i] = (end - start) * 1000.0;

        // Copy local part
        memcpy(x_extended, v_local, local.my_M * sizeof(double));

        GET_TIME(start);
        ghost_local_SpMV(&local_matrix, x_extended, y, local.my_M);
        GET_TIME(end);

        times_comp[i] = (end - start) * 1000.0;
    }

    double checksum = 0.0;
    for(int i=0; i<local.my_M; i++){
        checksum += y[i];
    }
    // To avoid compiler optimization
    if (checksum == 0.0 && rank == 0) printf("Warning: zero result\n");

    MPI_Barrier(MPI_COMM_WORLD);

    qsort(times_comm, iterations, sizeof(double), compare_doubles);
    qsort(times_comp, iterations, sizeof(double), compare_doubles);

    double p90_comm = percentile(times_comm, iterations, 0.90);
    double p90_comp = percentile(times_comp, iterations, 0.90);

    // Calculate the max time among all processes
    //MPI_Reduce( const void* sendbuf , void* recvbuf , MPI_Count count , MPI_Datatype datatype , MPI_Op op , int root , MPI_Comm comm);

    double max_comm = 0.0;
    double max_comp = 0.0;
    MPI_Reduce(&p90_comm, &max_comm, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&p90_comp, &max_comp, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    double local_mem = (local.my_nz * sizeof(double) + local.my_nz * sizeof(int) + (local.my_M+1) * sizeof(int) + local.my_M * sizeof(double)) / 1024.0 / 1024.0;
    double global_mem;
    MPI_Reduce(&local_mem, &global_mem, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // NNZ per rank (min,max,avg)
    int min_nz, max_nz, sum_nz;
    MPI_Reduce(&local.my_nz, &min_nz, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local.my_nz, &max_nz, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&local.my_nz, &sum_nz, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Communication volume per rank
    long long comm_volume_bytes = ghost_get_total_ghosts(&gp) * sizeof(double);

    long long max_comm_volume;
    MPI_Reduce(&comm_volume_bytes, &max_comm_volume, 1, MPI_LONG_LONG, MPI_MAX, 0, MPI_COMM_WORLD);


    if (rank == 0) {
        //gflops only on computation time
        double gflops = (2.0 * local.NZ_global) / ((max_comp) / 1000.0) / 1e9; //in seconds
        
        double avg_nz = (double)sum_nz / size;

        double total_time = max_comm + max_comp; 

        /*
        printf("Max_Comm: %f Max_Comp: %f Tot_time: %f \n", max_comm, max_comp, total_time);
        printf("GFLOPS: %f \n", gflops);
        printf("Mem_MB: %f \n", global_mem);
        printf("NZ_Min: %d \n", min_nz);
        printf("NZ_Max: %d \n", max_nz);
        printf("NZ_Avg: %f \n", avg_nz);
        printf("Comm_Volume_MB: %f \n", max_comm_volume / (1024.0 * 1024.0)); //conversion to mb
        */
        printf("STATS: %f %f %f %f %f %d %d %f %f \n", max_comm, max_comp, total_time, gflops, global_mem, min_nz, max_nz, avg_nz, max_comm_volume/(1024.0*1024.0));
    }

    ghost_free(&gp);
    local_matrix_free(&local);
    free(v_local);
    free(y);
    free(recvcounts_vec);
    free(displs_vec);
    free(times_comm);
    free(times_comp);
    free(x_extended);
    MPI_Finalize();
    return 0;
}