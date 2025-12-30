import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker

script_dir = os.path.dirname(os.path.abspath(__file__)) 
parent_dir = os.path.dirname(script_dir) 

INPUT_FILE = os.path.join(parent_dir, 'results', 'strong_scaling.csv') 
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'strong_scaling_main.png'

MATRIX_ORDER = [
    'venkat25.mtx',
    'atmosmodl.mtx',
    'rajat31.mtx',
    'circuit5M.mtx',
    'cage15.mtx'
]

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

print(f"Reading {INPUT_FILE}...")

try:
    df = pd.read_csv(INPUT_FILE)
    if len(df.columns) < 3: 
        df = pd.read_csv(INPUT_FILE, delimiter=';')
except Exception as e:
    print(f"Error reading CSV: {e}")
    exit(1)

df.columns = df.columns.str.strip()
df['Matrix'] = df['Matrix'].str.strip()
df['Processes'] = pd.to_numeric(df['Processes'], errors='coerce')
df['T_Comm'] = pd.to_numeric(df['T_Comm'], errors='coerce')
df['T_Comp'] = pd.to_numeric(df['T_Comp'], errors='coerce')
df = df.dropna()

# Use T_Comp for Speedup calculation based on the user's snippet logic
df['Speedup'] = 0.0
for mat in df['Matrix'].unique():
    subset = df[df['Matrix'] == mat]
    t1_row = subset[subset['Processes'] == 1]
    
    if not t1_row.empty:
        t1 = t1_row['T_Comp'].values[0]
        mask = df['Matrix'] == mat
        df.loc[mask, 'Speedup'] = t1 / df.loc[mask, 'T_Comp']

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

plotted_matrices = []
ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]

for mat in MATRIX_ORDER:
    if mat in df['Matrix'].unique():
        subset = df[df['Matrix'] == mat].sort_values('Processes')
        ax1.plot(subset['Processes'], subset['T_Comp'], marker='o', label=mat)
        ax2.plot(subset['Processes'], subset['Speedup'], marker='o', label=mat)
        plotted_matrices.append(mat)

# Plot 1: Execution Time
ax1.set_title('Strong Scaling: Execution Time', fontsize=14)
ax1.set_xlabel('Number of Processes', fontsize=12)
ax1.set_ylabel('Time (s)', fontsize=12)  
ax1.set_xscale('log')
ax1.set_yscale('log')
ax1.set_xticks(ticks_x)
ax1.set_xticklabels(ticks_x) 
ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter()) 
ax1.grid(True, which="both", ls="-", alpha=0.5)
ax1.legend(plotted_matrices, title="Matrices")

# Plot 2: Speedup
ideal_x = ticks_x
ax2.plot(ideal_x, ideal_x, 'k--', label='Ideal', linewidth=2)

ax2.set_title('Strong Scaling: Speedup', fontsize=14)
ax2.set_xlabel('Number of Processes', fontsize=12)
ax2.set_ylabel('Speedup (T1/Tp)', fontsize=12)
ax2.set_xscale('log')
ax2.set_xticks(ideal_x)
ax2.set_xticklabels(ideal_x)
ax2.grid(True, which="both", ls="-", alpha=0.5)
ax2.legend(plotted_matrices + ['Ideal'], title="Matrices")

out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.tight_layout()
plt.savefig(out_path)
print(f"Successfully saved plot to {out_path}")