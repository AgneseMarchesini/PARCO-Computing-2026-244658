import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker
import numpy as np

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

# Read both CSV files
INPUT_FILE_DISTVEC = os.path.join(parent_dir, 'results', 'strong_scaling_distvec.csv')
INPUT_FILE_GHOST = os.path.join(parent_dir, 'results', 'strong_scaling_ghost.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')

MATRIX_ORDER = [
    'venkat25.mtx',
    'atmosmodl.mtx',
    'rajat31.mtx',
    'circuit5M.mtx',
    'cage15.mtx'
]

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# Function to load and process data
def load_data(input_file, mode_name):
    print(f"Reading {input_file}...")
    try:
        df = pd.read_csv(input_file)
        if len(df.columns) < 3:
            df = pd.read_csv(input_file, delimiter=';')
    except FileNotFoundError:
        print(f"WARNING: Could not find file at: {input_file}")
        return None
    except Exception as e:
        print(f"Error reading CSV: {e}")
        return None
    
    df.columns = df.columns.str.strip()
    df['Matrix'] = df['Matrix'].str.strip()
    df['Processes'] = pd.to_numeric(df['Processes'], errors='coerce')
    df['T_Comm'] = pd.to_numeric(df['T_Comm'], errors='coerce')
    df['T_Comp'] = pd.to_numeric(df['T_Comp'], errors='coerce')
    df = df.dropna()
    
    # CORRECTED: Calculate Total Time
    df['Total'] = df['T_Comp'] + df['T_Comm']
    
    df['Speedup'] = 0.0
    df['Efficiency'] = 0.0
    
    # Calculate speedup and efficiency using TOTAL TIME
    for mat in df['Matrix'].unique():
        subset = df[df['Matrix'] == mat]
        t1_row = subset[subset['Processes'] == 1]
        if not t1_row.empty:
            t1_total = t1_row['Total'].values[0]  # Use Total Time
            mask = df['Matrix'] == mat
            
            # Speedup = T1_total / Tp_total
            df.loc[mask, 'Speedup'] = t1_total / df.loc[mask, 'Total']
            # Efficiency = Speedup / P
            df.loc[mask, 'Efficiency'] = df.loc[mask, 'Speedup'] / df.loc[mask, 'Processes']
    
    return df

# Function to create plots for a single mode
def create_plot(df, mode_name, output_filename):
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 6))
    plotted_matrices = []
    ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]
    
    for mat in MATRIX_ORDER:
        if mat in df['Matrix'].unique():
            subset = df[df['Matrix'] == mat].sort_values('Processes')
            
            # Plot 1: Total Execution Time (CORRECTED)
            ax1.plot(subset['Processes'], subset['Total'], marker='o', label=mat, linewidth=2, markersize=6)
            
            # Plot 2: Speedup (based on Total Time)
            ax2.plot(subset['Processes'], subset['Speedup'], marker='o', label=mat, linewidth=2, markersize=6)
            
            # Plot 3: Efficiency
            ax3.plot(subset['Processes'], subset['Efficiency'], marker='o', label=mat, linewidth=2, markersize=6)
            
            plotted_matrices.append(mat)
    
    # ========== Plot 1: Total Execution Time (LINEAR SCALE) ==========
    ax1.set_title(f'Strong Scaling: Total Execution Time ({mode_name})', fontsize=14, fontweight='bold')
    ax1.set_xlabel('Number of Processes', fontsize=12)
    ax1.set_ylabel('Total Time (ms)', fontsize=12)
    ax1.set_xscale('log')
    ax1.set_xticks(ticks_x)
    ax1.set_xticklabels(ticks_x)
    ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax1.grid(True, which="both", ls="-", alpha=0.3)
    ax1.legend(plotted_matrices, title="Matrices", loc='best')
    
    # ========== Plot 2: Speedup (LINEAR SCALE) ==========
    # Add ideal speedup line
    ideal_x = np.array(ticks_x)
    ax2.plot(ideal_x, ideal_x, 'k--', label='Ideal', linewidth=2, alpha=0.7)
    
    ax2.set_title(f'Strong Scaling: Speedup ({mode_name})', fontsize=14, fontweight='bold')
    ax2.set_xlabel('Number of Processes', fontsize=12)
    ax2.set_ylabel('Speedup (T₁ / Tₚ)', fontsize=12)
    ax2.set_xscale('log')
    ax2.set_xticks(ticks_x)
    ax2.set_xticklabels(ticks_x)
    ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax2.grid(True, which="both", ls="-", alpha=0.3)
    ax2.legend(plotted_matrices + ['Ideal'], title="Matrices", loc='best')
    
    # ========== Plot 3: Efficiency ==========
    ax3.axhline(y=1.0, color='k', linestyle='--', label='Ideal (1.0)', linewidth=2, alpha=0.7)
    
    ax3.set_title(f'Strong Scaling: Efficiency ({mode_name})', fontsize=14, fontweight='bold')
    ax3.set_xlabel('Number of Processes', fontsize=12)
    ax3.set_ylabel('Efficiency (Speedup / P)', fontsize=12)
    ax3.set_xscale('log')
    ax3.set_xticks(ticks_x)
    ax3.set_xticklabels(ticks_x)
    ax3.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax3.set_ylim(0, 1.1)
    ax3.grid(True, which="both", ls="-", alpha=0.3)
    ax3.legend(plotted_matrices + ['Ideal'], title="Matrices", loc='best')
    
    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, output_filename)
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    print(f"Successfully saved plot to {out_path}")
    plt.close()

# Load both datasets
df_distvec = load_data(INPUT_FILE_DISTVEC, 'Distributed Vector')
df_ghost = load_data(INPUT_FILE_GHOST, 'Ghost Entries')

# Generate plots
if df_distvec is not None:
    create_plot(df_distvec, 'Distributed Vector', 'strong_scaling_distvec.png')
    
if df_ghost is not None:
    create_plot(df_ghost, 'Ghost Entries', 'strong_scaling_ghost.png')

if df_distvec is None and df_ghost is None:
    print("CRITICAL ERROR: No data files found!")
    exit(1)

print("\nAll strong scaling plots generated successfully!")
