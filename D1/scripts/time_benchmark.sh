#!/bin/bash

EXECUTABLE="./spmv"
MATRICES=(
    "af23560.mtx"
    "twotone.mtx"
    "tmt_unsym.mtx"
    "atmosmodl.mtx"
    "Freescale1.mtx"
)
DATA_DIR="D1/data"
MODES=(
    #"seq" handled separately
    "static"
    "dynamic"
    "guided"
)
CHUNK_SIZE=(1 10 100 1000)
THREADS=(1 2 4 8 16 32 64)
NUM_RUNS=10
RESULTS_DIR="D1/results"
RESULTS_CSV="$RESULTS_DIR/time_results.csv"

# Compile the program
gcc -Wall -O3 -fopenmp D1/src/main.c D1/src/matrix.c D1/src/mmio.c -o spmv -lm

# Clear results file
echo "Matrix,Mode,ChunkSize,Threads,Time_ms" > $RESULTS_CSV

for MATRIX_NAME in "${MATRICES[@]}"
do
    MATRIX_FILE="$DATA_DIR/$MATRIX_NAME"

    # Sequential version
    echo "Running: $MATRIX_NAME, seq, NA, 1 thread" # matrix_name, mode, chunk_size, threads
    
    # spmv matrix_file mode chunk_size threads
    TIME_MS=$($EXECUTABLE $MATRIX_FILE seq 1 1 $NUM_RUNS | grep "Time:" | awk '{print $2}')
   
    echo "$MATRIX_NAME,seq,NA,1,$TIME_MS" >> $RESULTS_CSV

    # Loop over modes
    for MODE in "${MODES[@]}"
    do
        # Loop over chunk sizes
        for CHUNK in "${CHUNK_SIZE[@]}"
        do
            # Loop over threads
            for T in "${THREADS[@]}"
            do 
                echo "Running: $MATRIX_NAME, $MODE, $CHUNK, $T threads"

                TIME_MS=$($EXECUTABLE $MATRIX_FILE $MODE $CHUNK $T $NUM_RUNS | grep "Time:" | awk '{print $2}')

                echo "$MATRIX_NAME,$MODE,$CHUNK,$T,$TIME_MS" >> $RESULTS_CSV
            done
        done
    done
done

echo "Data saved in $RESULTS_CSV"