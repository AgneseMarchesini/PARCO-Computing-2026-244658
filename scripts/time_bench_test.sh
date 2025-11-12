#!/bin/bash

EXECUTABLE="./spmv"
MATRICES=(
    "af23560.mtx"
    #"twotone.mtx"
    #"tmt_unsym.mtx"
    #"atmosmodl.mtx"
    #"Freescale1.mtx"
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
TEMP_FILE="temp_times.txt"


# Compile the program
gcc -Wall -O3 -fopenmp src/sparse_matrix_x_vector.c src/matrix.c src/mmio.c -o spmv

# Clear results file
echo "Matrix,Mode,ChunkSize,Threads,Time_ms" > $RESULTS_CSV

for MATRIX_NAME in "${MATRICES[@]}"
do
    MATRIX_FILE="$DATA_DIR/$MATRIX_NAME"

    # Sequential version
    echo "Running: $MATRIX_NAME, seq, NA, 1 thread" # matrix_name, mode, chunk_size, threads
    
    > $TEMP_FILE # Clear temp file
    # Run sequential multiple times 
    for (( r=1; r<=$NUM_RUNS; r++ ))
    do
        # spmv matrix_file mode chunk_size threads
        $EXECUTABLE $MATRIX_FILE seq 1 1 | grep "Time:" | awk '{print $2}' >> $TEMP_FILE
    done

    # Calculate 90th percentile for sequential runs
    # sort -n sorts
    # head -n 9 gets first 9 lines
    # tail -n 1 gets the 9th run (90th percentile)
    TIME_MS=$(sort -n $TEMP_FILE | head -n 9 | tail -n 1)
    
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

                # Run the parallel version multiple times like before
                > $TEMP_FILE # Clear temp file
                for (( r=1; r<=$NUM_RUNS; r++ ))
                do
                    $EXECUTABLE $MATRIX_FILE $MODE $CHUNK $T | grep "Time:" | awk '{print $2}' >> $TEMP_FILE
                done

                TIME_MS=$(sort -n $TEMP_FILE | head -n 9 | tail -n 1)

                echo "$MATRIX_NAME,$MODE,$CHUNK,$T,$TIME_MS" >> $RESULTS_CSV
            done
        done
    done
done

rm $TEMP_FILE

echo "Data saved in $RESULTS_CSV"