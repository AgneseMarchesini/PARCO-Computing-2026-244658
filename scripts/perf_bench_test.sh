#!/bin/bash

EXECUTABLE="./spmv.exe"
MATRICES=(
    "af23560.mtx"
    #"twotone.mtx"
    #"tmt_unsym.mtx"
    #"atmosmodl.mtx"
    #"Freescale1.mtx"
)
DATA_DIR="data"
MODES=(
    "static"
    "dynamic"
    "guided"
)
CHUNK_SIZE=(1 10 100 1000)
THREADS=(1 2 4 8 16 32 64)
NUM_RUNS=1 
RESULTS_DIR="results"
PERF_CSV="$RESULTS_DIR/perf_results.csv"

# Events to measure
PERF_EVENTS="cache-misses,cache-references,LLC-load-misses,instructions,cycles"

gcc -Wall -O3 -fopenmp src/main.c src/matrix.c src/mmio.c -o spmv -lm

# Setup
mkdir -p $RESULTS_DIR

echo "Matrix,Mode,ChunkSize,Threads,Cycles,Instructions,CacheMisses,CacheRefs,LLCMisses" > $PERF_CSV

for MATRIX_NAME in "${MATRICES[@]}"
do
    MATRIX_FILE="$DATA_DIR/$MATRIX_NAME"

    # Sequential version 
    echo "Running PERF: $MATRIX_NAME, seq, NA, 1 thread"

    PERF_OUTPUT=$(perf stat -e $PERF_EVENTS -- \
        $EXECUTABLE $MATRIX_FILE seq 1 1 $NUM_RUNS \
        2>&1 >/dev/null)


    CYCLES=$(echo "$PERF_OUTPUT" | grep "cycles" | awk '{print $1}' | sed 's/,//g')
    INSTRUCTIONS=$(echo "$PERF_OUTPUT" | grep "instructions" | awk '{print $1}' | sed 's/,//g')
    CACHE_MISSES=$(echo "$PERF_OUTPUT" | grep "cache-misses" | awk '{print $1}' | sed 's/,//g')
    CACHE_REFS=$(echo "$PERF_OUTPUT" | grep "cache-references" | awk '{print $1}' | sed 's/,//g')
    LLC_MISSES=$(echo "$PERF_OUTPUT" | grep "LLC-load-misses" | awk '{print $1}' | sed 's/,//g')

    echo "$MATRIX_NAME,seq,NA,1,$CYCLES,$INSTRUCTIONS,$CACHE_MISSES,$CACHE_REFS,$LLC_MISSES" >> $PERF_CSV


    # Parallel versions
    for MODE in "${MODES[@]}"
    do
        for CHUNK in "${CHUNK_SIZE[@]}"
        do
            for T in "${THREADS[@]}"
            do 
                echo "Running PERF: $MATRIX_NAME, $MODE, $CHUNK, $T threads"
                
                PERF_OUTPUT=$(perf stat -e $PERF_EVENTS -- \
                    $EXECUTABLE $MATRIX_FILE $MODE $CHUNK $T $NUM_RUNS \
                    2>&1 >/dev/null)
                
                CYCLES=$(echo "$PERF_OUTPUT" | grep "cycles" | awk '{print $1}' | sed 's/,//g')
                INSTRUCTIONS=$(echo "$PERF_OUTPUT" | grep "instructions" | awk '{print $1}' | sed 's/,//g')
                CACHE_MISSES=$(echo "$PERF_OUTPUT" | grep "cache-misses" | awk '{print $1}' | sed 's/,//g')
                CACHE_REFS=$(echo "$PERF_OUTPUT" | grep "cache-references" | awk '{print $1}' | sed 's/,//g')
                LLC_MISSES=$(echo "$PERF_OUTPUT" | grep "LLC-load-misses" | awk '{print $1}' | sed 's/,//g')

                echo "$MATRIX_NAME,$MODE,$CHUNK,$T,$CYCLES,$INSTRUCTIONS,$CACHE_MISSES,$CACHE_REFS,$LLC_MISSES" >> $PERF_CSV
            done
        done
    done
done

echo "Performance data saved in $PERF_CSV"