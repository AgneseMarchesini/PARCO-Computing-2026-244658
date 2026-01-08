#ifndef REPLVEC_H
#define REPLVEC_H

#include <mpi.h>

/// Build recvcounts and displs for a distributed vector
void repl_build_counts_displs(int size, int my_M, int *recvcounts, int *displs, MPI_Comm comm);

#endif
