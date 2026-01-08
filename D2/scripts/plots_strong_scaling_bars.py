import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np
import matplotlib.colors as mcolors
from matplotlib.patches import Patch

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

# Read both CSV files
INPUT_FILE_DISTVEC = os.path.join(parent_dir, 'results', 'strong_scaling_distvec.csv')
INPUT_FILE_GHOST = os.path.join(parent_dir, 'results', 'strong_scaling_ghost.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'strong_scaling_bars_comparison.png'

# Matrix list
MATRIX_ORDER = [
    'venkat25.mtx',
    'atmosmodl.mtx',
    'rajat31.mtx',
    'circuit5M.mtx',
    'cage15.mtx'
]

# Define unique base colors for each matrix
MATRIX_COLORS = {
    'venkat25.mtx': '#1f77b4',  # Blue
    'atmosmodl.mtx': '#ff7f0e',  # Orange
    'rajat31.mtx': '#2ca02c',  # Green
    'circuit5M.mtx': '#d62728',  # Red
    'cage15.mtx': '#9467bd'  # Purple
}

def darken_color(color, factor=0.6):
    """Returns a darker version of the given color."""
    try:
        c = mcolors.to_rgb(color)
        return mcolors.to_hex([max(0, x * factor) for x in c])
    except:
        return color

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# Function to load data
def load_data(input_file):
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
    
    # Values are already in Milliseconds
    df['Total_Time'] = df['T_Comm'] + df['T_Comp']
    
    return df

# Load both datasets
df_distvec = load_data(INPUT_FILE_DISTVEC)
df_ghost = load_data(INPUT_FILE_GHOST)

if df_distvec is None or df_ghost is None:
    print("CRITICAL ERROR: One or both data files not found!")
    exit(1)

# Create 2 Rows x 5 Columns (Row 1 = Distributed Vector, Row 2 = Ghost Entries)
fig, axes = plt.subplots(2, 5, figsize=(25, 10))

# Row 0: Distributed Vector
# Row 1: Ghost Entries
datasets = [
    (df_distvec, 'Distributed Vector'),
    (df_ghost, 'Ghost Entries')
]

for row_idx, (df, mode_name) in enumerate(datasets):
    for col_idx, mat in enumerate(MATRIX_ORDER):
        ax = axes[row_idx, col_idx]
        
        # Check if matrix exists in data
        if mat in df['Matrix'].unique():
            subset = df[df['Matrix'] == mat].sort_values('Processes')
            
            # Prepare Data
            processes = subset['Processes'].astype(str).tolist()
            x_indices = np.arange(len(processes))
            bar_width = 0.6
            
            # Colors
            base_color = MATRIX_COLORS.get(mat, '#333333')
            darker_color = darken_color(base_color)
            
            # 1. Computation (Base Color) - Bottom
            ax.bar(x_indices, subset['T_Comp'], width=bar_width,
                   color=base_color, edgecolor='white', label='Computation')
            
            # 2. Communication (Darker Color) - Stacked Top
            ax.bar(x_indices, subset['T_Comm'], bottom=subset['T_Comp'],
                   width=bar_width, color=darker_color, edgecolor='white', label='Communication')
            
            # Formatting
            # Title: Matrix name only on top row, mode name on left
            if row_idx == 0:
                ax.set_title(mat, fontsize=14, fontweight='bold')
            
            ax.set_xlabel('Processes', fontsize=11)
            
            # Y-label: Only first column + mode name
            if col_idx == 0:
                ax.set_ylabel(f'{mode_name}\nTime (ms)', fontsize=12, fontweight='bold')
            
            ax.set_xticks(x_indices)
            ax.set_xticklabels(processes)
            
            # Linear Scale
            ax.grid(axis='y', linestyle='--', alpha=0.5)
            
            # Legend (Computation vs Communication)
            legend_elements = [
                Patch(facecolor=base_color, label='Comp'),
                Patch(facecolor=darker_color, label='Comm')
            ]
            ax.legend(handles=legend_elements, loc='upper left', fontsize=9)
        else:
            # Hide axis if matrix data is missing
            ax.axis('off')

plt.tight_layout()
out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.savefig(out_path, dpi=300, bbox_inches='tight')
print(f"Successfully saved comparison plot to {out_path}")
