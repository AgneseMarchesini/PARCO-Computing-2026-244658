#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"

int compare( const void* a, const void* b)
{
     int int_a = * ( (int*) a );
     int int_b = * ( (int*) b );
     
     if ( int_a == int_b ) return 0;
     else if ( int_a < int_b ) return -1;
     else return 1;
}

int main(int argc, char *argv[]) {
    FILE *f;
    MM_typecode matcode;
    int M, N, nz;            // rows, cols, nonzeros
    int i;
    int *I, *J;
    double *val;

    if (argc < 2) {
        printf("Usage: %s matrix_file.mtx\n", argv[0]);
        return 1;
    }

    // Open the file
    if ((f = fopen(argv[1], "r")) == NULL) {
        perror("fopen");
        return 1;
    }

    // Read the banner (first line)
    if (mm_read_banner(f, &matcode) != 0) {
        printf("Could not process Matrix Market banner.\n");
        return 1;
    }

    // Read the size (rows, cols, nnz)
    if (mm_read_mtx_crd_size(f, &M, &N, &nz) != 0)
        return 1;

    // Allocate space
    I = (int *) malloc(nz * sizeof(int));
    J = (int *) malloc(nz * sizeof(int));
    val = (double *) malloc(nz * sizeof(double));

    // Read triplets
    for (i = 0; i < nz; i++) {
        fscanf(f, "%d %d %lf", &I[i], &J[i], &val[i]);
        I[i]--;  // convert to 0-based indexing (C uses 0-based)
        J[i]--;
    }
    
    // Now we need to convert from COO to CSR coordinates
    // the column index array and the value array are the same, we need to change the row index to row_pnt
    
    // sort the row index array
    qsort( I, nz, sizeof(int), compare );
    
    // init of row_pnt
    int* row_pnt = (int *)malloc((M+1) * sizeof(int));
    
    // Count Non-Zeros Per Row: basically we use row_pnt to count, for example if the sparse matrix has 2 non-zero elemnts in the first row, we increment the position 1 of row_pnt two times, it's a counter
    for(int i = 0; i < nz; i++){
        row_pnt[I[i] + 1]++; 
    }
    
    // convert the row counts into cumulative sums
    row_pnt[0] = 0;
    for(int i = 0; i < M; i++){
      row_pnt[i+1] += row_pnt[i]; //current position = previous position + number of nz
    }
    
    /*
    for(int i = 0; i < M; i++){
      printf("%d ", row_pnt[i]);
    }*/
    
    
    fclose(f);

    printf("Loaded %d x %d matrix with %d nonzeros\n", M, N, nz);
    printf("Example: A[%d, %d] = %g\n", row_pnt[0], J[0], val[0]);

    // Free memory
    free(I);
    free(J);
    free(val);
    free(row_pnt);

    return 0;
}
