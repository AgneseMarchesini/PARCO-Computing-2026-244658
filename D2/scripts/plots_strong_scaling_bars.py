import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np
import matplotlib.colors as mcolors
from matplotlib.patches import Patch

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

INPUT_FILE = os.path.join(parent_dir, 'results', 'strong_scaling.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'strong_scaling_bars.png'

MATRIX_ORDER = [
    'venkat25.mtx',
    'atmosmodl.mtx',
    'rajat31.mtx',
    'circuit5M.mtx',
    'cage15.mtx'
]

MATRIX_COLORS = {
    'venkat25.mtx': '#1f77b4',
    'atmosmodl.mtx': '#ff7f0e',
    'rajat31.mtx': '#2ca02c',
    'circuit5M.mtx': '#d62728',
    'cage15.mtx': '#9467bd'
}

def darken_color(color, factor=0.6):
    try:
        c = mcolors.to_rgb(color)
        return mcolors.to_hex([max(0, x * factor) for x in c])
    except:
        return color

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

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
    df['Total_Time'] = df['T_Comm'] + df['T_Comp']
    return df

df = load_data(INPUT_FILE)
if df is None:
    print("CRITICAL ERROR: Data file not found!")
    exit(1)

fig, axes = plt.subplots(1, 5, figsize=(25, 5))

for col_idx, mat in enumerate(MATRIX_ORDER):
    ax = axes[col_idx]
    
    if mat in df['Matrix'].unique():
        subset = df[df['Matrix'] == mat].sort_values('Processes')
        
        processes = subset['Processes'].astype(str).tolist()
        x_indices = np.arange(len(processes))
        bar_width = 0.6
        
        base_color = MATRIX_COLORS.get(mat, '#333333')
        darker_color = darken_color(base_color)
        
        ax.bar(x_indices, subset['T_Comp'], width=bar_width,
               color=base_color, edgecolor='white', label='Computation')
        
        ax.bar(x_indices, subset['T_Comm'], bottom=subset['T_Comp'],
               width=bar_width, color=darker_color, edgecolor='white', label='Communication')
        
        ax.set_title(mat, fontsize=14, fontweight='bold')
        ax.set_xlabel('Processes', fontsize=11)
        
        if col_idx == 0:
            ax.set_ylabel('Time (s)', fontsize=12, fontweight='bold')
        
        ax.set_xticks(x_indices)
        ax.set_xticklabels(processes)
        ax.grid(axis='y', linestyle='--', alpha=0.5)
        
        legend_elements = [
            Patch(facecolor=base_color, label='Comp'),
            Patch(facecolor=darker_color, label='Comm')
        ]
        ax.legend(handles=legend_elements, loc='upper right', fontsize=9)
    else:
        ax.axis('off')

plt.tight_layout()
out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.savefig(out_path, dpi=300, bbox_inches='tight')
print(f"Successfully saved plot to {out_path}")
