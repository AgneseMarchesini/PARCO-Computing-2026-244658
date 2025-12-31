import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np
import matplotlib.colors as mcolors

script_dir = os.path.dirname(os.path.abspath(__file__)) 
parent_dir = os.path.dirname(script_dir) 

INPUT_FILE = os.path.join(parent_dir, 'results', 'strong_scaling.csv') 
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'strong_scaling_bars.png'

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
    'venkat25.mtx': '#1f77b4',      # Blue
    'atmosmodl.mtx': '#ff7f0e',     # Orange
    'rajat31.mtx': '#2ca02c',       # Green
    'circuit5M.mtx': '#d62728',     # Red
    'cage15.mtx': '#9467bd'         # Purple
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

# Values are already in Milliseconds
df['Total_Time'] = df['T_Comm'] + df['T_Comp']

# Create 1 Row containing 5 Plots (Side by Side)
# Increase width (25) to make sure they are not squashed
fig, axes = plt.subplots(1, 5, figsize=(25, 6))

# Helper to ensure axes is iterable even if only 1 plot
if len(MATRIX_ORDER) == 1:
    axes = [axes]

for i, mat in enumerate(MATRIX_ORDER):
    ax = axes[i]
    
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
        ax.set_title(mat, fontsize=14, fontweight='bold')
        ax.set_xlabel('Processes', fontsize=11)
        
        # Only set Y-label for the first plot to save space, or all if preferred
        if i == 0:
            ax.set_ylabel('Time (ms)', fontsize=12)
        
        ax.set_xticks(x_indices)
        ax.set_xticklabels(processes)
        
        # Linear Scale
        ax.grid(axis='y', linestyle='--', alpha=0.5)
        
        # Legend (Computation vs Communication)
        from matplotlib.patches import Patch
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
plt.savefig(out_path)
print(f"Successfully saved combined side-by-side plot to {out_path}")