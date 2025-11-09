#!/bin/bash

EXECUTABLE="./spmv"
MATRICES=(
    "af23560.mtx"
    #"fidapm11.mtx"
    #"cfd2.mtx"
    #"s3dkt3m2.mtx"
    #"pwtk.mtx"
)
DATA_DIR="data"
MODES=(
    #"seq" handled separately
    "static"
    "dynamic"
    "guided"
)
CHUNK_SIZE=(1 10 100 1000 10000)
THREADS=(1 2 4 8 16 32 64)
NUM_RUNS=10
RESULTS_DIR="results"
RESULTS_CSV="$RESULTS_DIR/benchmark_results.csv"


# Compile the program
gcc -Wall -g -O3 -fopenmp src/sparse_matrix_x_vector.c src/matrix.c src/mmio.c -o spmv

# Clear results file
echo "Matrix,Mode,ChunkSize,Threads,Time_ms" > $RESULTS_CSV

for MATRIX_NAME in "${MATRICES[@]}"
do
    MATRIX_FILE="$DATA_DIR/$MATRIX_NAME"

    # Sequential version
    export OMP_NUM_THREADS=1
    echo "Running: $MATRIX_NAME, seq, NA, 1 thread" # matrix_name, mode, chunk_size, threads

    OUTPUT_LINE=$($EXECUTABLE $MATRIX_FILE seq 10 1 | grep "90th percentile") # matrix_name, mode, num_runs, chunk_size
    TIMES_MS=$(echo $OUTPUT_LINE | awk '{print $4}')
    echo "$MATRIX_NAME,seq,NA,1,$TIMES_MS" >> $RESULTS_CSV

    # Loop over modes
    for MODE in "${MODES[@]}"
    do
        # Loop over chunk sizes
        for CHUNK in "${CHUNK_SIZE[@]}"
        do
            # Loop over threads
            for T in "${THREADS[@]}"
            do 
                export OMP_NUM_THREADS=$T

                echo "Running: $MATRIX_NAME, $MODE, $CHUNK, $T threads"

                # Find the "90th percentile" line in the output
                OUTPUT_LINE=$($EXECUTABLE $MATRIX_FILE $MODE $NUM_RUNS $CHUNK | grep "90th percentile")

                TIMES_MS=$(echo $OUTPUT_LINE | awk '{print $4}')

                echo "$MATRIX_NAME,$MODE,$CHUNK,$T,$TIMES_MS" >> $RESULTS_CSV
            done
        done
    done
done

echo "Data saved in $RESULTS_CSV"