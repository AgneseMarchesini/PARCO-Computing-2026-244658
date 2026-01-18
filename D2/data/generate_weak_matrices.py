# run this script in the root folder PARCO-2026-2445658
# python3 .\D2\data\generate_weak_matrices.py

import os
import random


output_dir = "D2/data/weak_scaling_matrices"  
base_rows = 256                       
nnz_per_row = 10                      
procs = [1, 2, 4, 8, 16, 32, 64, 128]

if not os.path.exists(output_dir):
    os.makedirs(output_dir)

print(f"Generating matrices in '{output_dir}'")
print(f"Configuration: {nnz_per_row} Non-Zeros per Row")

for p in procs:
    rows = base_rows * p
    cols = rows
    
    total_estimated_nnz = rows * nnz_per_row
    
    filename = f"weak_{p}p_{rows}.mtx"
    filepath = os.path.join(output_dir, filename)
    
    with open(filepath, 'w') as f:
        f.write("%%MatrixMarket matrix coordinate real general\n")
        f.write(f"{rows} {cols} {total_estimated_nnz}\n")
        
        for r in range(1, rows + 1):
            f.write(f"{r} {r} {random.uniform(1, 10):.6f}\n")
            
            cols_to_add = random.sample([c for c in range(1, cols+1) if c != r], nnz_per_row - 1)
            
            for c in cols_to_add:
                val = random.uniform(0.1, 1.0)
                f.write(f"{r} {c} {val:.6f}\n")

    print(f"  - Generated {filename}: {rows} rows, ~{total_estimated_nnz} NNZ")