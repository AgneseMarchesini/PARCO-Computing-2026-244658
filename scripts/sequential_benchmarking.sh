#!/bin/bash

EXECUTABLE="./spmv"
MATRICES=(
    "af23560.mtx"
    "fidapm11.mtx"
    "s3dkt3m2.mtx"
    "cfd2.mtx"
    "pwtk.mtx"
)
MODES=(
    "seq"
    "static"
    "dynamic"
    "guided"
)
THREADS=(1 2 4 8 16 32 64)
RESULTS_CSV="benchmark_results.csv"

# Compile the program
gcc -Wall -g -O3 -fopenmp sparse_matrix_x_vector.c matrix.c mmio.c -o spmv

# Clear results file
echo "Matrix,Mode,Threads,Time_ms" > $RESULTS_CSV

for MATRIX_FILE in "${MATRICES[@]}"
do
    # Loop over modes
    for MODE in "${MODES[@]}"
    do
        # Loop over threads
        for T in "${THREADS[@]}"
        do 
            # If the mode is sequential, only run with 1 thread
            if [ "$MODE" == "seq" ] && [ $T -gt 1 ]; then
                continue
            fi 

            export OMP_NUM_THREADS=$T

            # Find the "90th percentile" line in the output
            OUTPUT_LINE=$($EXECUTABLE $MATRIX_FILE $MODE | grep "90th percentile")

            TIMES_MS=$(echo $OUTPUT_LINE | awk '{print $4}')

            echo "$MATRIX_FILE,$MODE,$T,$TIMES_MS" >> $RESULTS_CSV
        done
    done
done

echo "Data saved in $RESULTS_CSV"