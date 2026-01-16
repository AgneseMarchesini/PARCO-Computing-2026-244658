#!/bin/bash

# Remember chmod +x D2/scripts/benchmark_strong_scaling.sh

EXECUTABLE="./spmv"
DATA_DIR="D2/data/weak_scaling_matrices"
RESULTS_DIR="D2/results"
WEAK_SCALING_CSV="$RESULTS_DIR/weak_scaling_local.csv"

mpicc -O3 -Wall D2/src/main.c D2/src/matrix.c D2/src/mmio.c D2/src/ghost_entries.c D2/src/distribution.c -o spmv || { echo "Compilation failed"; exit 1; }
chmod +x ./spmv

# Setup
mkdir -p $RESULTS_DIR

echo "Matrix,Processes,T_Comm,T_Comp,Tot_Time,GFLOPS,Mem_MB,NZ_Min,NZ_Max,NZ_Avg,Volume_MB" > $WEAK_SCALING_CSV

for P in 1 2 4 8 16 32 64 128
do
    ROWS=$((256 * P))
    MATRIX_NAME="weak_${P}p_${ROWS}.mtx"
    echo "Running with $P processes... ($MATRIX_NAME)"

    OUTPUT=$(mpirun -np $P $EXECUTABLE "$DATA_DIR/$MATRIX_NAME")
    
    STATS_LINE=$(echo "$OUTPUT" | grep "STATS:")
        
        # printf("STATS: %f %f %f %f %f %d %d %f %f", max_comm, max_comp, total_time, gflops, global_mem, min_nz, max_nz, avg_nz, max_comm_volume/(1024.0*1024.0));

        TIME_COMM=$(echo "$STATS_LINE" | awk '{print $2}')
        TIME_COMP=$(echo "$STATS_LINE" | awk '{print $3}')
        TOT_TIME=$(echo "$STATS_LINE" | awk '{print $4}')
        GFLOPS=$(echo "$STATS_LINE" | awk '{print $5}')
        MEM_MB=$(echo "$STATS_LINE" | awk '{print $6}')
        NZ_MIN=$(echo "$STATS_LINE" | awk '{print $7}')
        NZ_MAX=$(echo "$STATS_LINE" | awk '{print $8}')
        NZ_AVG=$(echo "$STATS_LINE" | awk '{print $9}')
        VOLUME=$(echo "$STATS_LINE" | awk '{print $10}')

    echo "$MATRIX_NAME,$P,$TIME_COMM,$TIME_COMP,$TOT_TIME,$GFLOPS,$MEM_MB,$NZ_MIN,$NZ_MAX,$NZ_AVG,$VOLUME" >> $WEAK_SCALING_CSV
done
