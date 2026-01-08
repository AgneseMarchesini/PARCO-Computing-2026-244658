#include <mpi.h>
#include "replvec.h"

void repl_build_counts_displs(int size, int my_M, int *recvcounts, int *displs, MPI_Comm comm){
    MPI_Allgather(&my_M, 1, MPI_INT, recvcounts, 1, MPI_INT, comm);

    displs[0] = 0;
    for (int i = 1; i < size; i++) {
        displs[i] = displs[i-1] + recvcounts[i-1];
    }
}
