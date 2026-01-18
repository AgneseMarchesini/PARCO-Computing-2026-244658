import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

INPUT_WEAK = os.path.join(parent_dir, 'results', 'weak_scaling.csv')
INPUT_STRONG = os.path.join(parent_dir, 'results', 'strong_scaling.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'gflops_comparison.png'

MATRIX_ORDER = [
    'venkat25.mtx',
    'atmosmodl.mtx',
    'rajat31.mtx',
    'circuit5M.mtx',
    'cage15.mtx'
]

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
    df['Processes'] = pd.to_numeric(df['Processes'], errors='coerce')
    df['GFLOPS'] = pd.to_numeric(df['GFLOPS'], errors='coerce')
    df = df.dropna()
    return df

df_weak = load_data(INPUT_WEAK)
df_strong = load_data(INPUT_STRONG)

if df_weak is None or df_strong is None:
    print("CRITICAL ERROR: One or both data files not found!")
    exit(1)

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 7))
ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]

ax1.plot(df_weak['Processes'], df_weak['GFLOPS'],
         'ro-', linewidth=3, markersize=10, label='Weak Scaling')
ax1.set_title('Weak Scaling: GFLOPS', fontsize=16, fontweight='bold')
ax1.set_xlabel('Number of Processes', fontsize=14)
ax1.set_ylabel('GFLOPS', fontsize=14)
ax1.yaxis.set_major_locator(ticker.MaxNLocator(nbins=12))
ax1.set_xscale('log')
ax1.set_xticks(ticks_x)
ax1.set_xticklabels(ticks_x)
ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax1.grid(True, which="both", ls="-", alpha=0.3)
ax1.legend(loc='best', fontsize=12)

df_strong['Matrix'] = df_strong['Matrix'].str.strip()
for mat in MATRIX_ORDER:
    if mat in df_strong['Matrix'].unique():
        subset = df_strong[df_strong['Matrix'] == mat].sort_values('Processes')
        ax2.plot(subset['Processes'], subset['GFLOPS'],
                 marker='o', linewidth=2, markersize=6, label=mat)

ax2.set_title('Strong Scaling: GFLOPS', fontsize=16, fontweight='bold')
ax2.set_xlabel('Number of Processes', fontsize=14)
ax2.set_ylabel('GFLOPS', fontsize=14)
ax2.yaxis.set_major_locator(ticker.MaxNLocator(nbins=12))
ax2.set_xscale('log')
ax2.set_xticks(ticks_x)
ax2.set_xticklabels(ticks_x)
ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax2.grid(True, which="both", ls="-", alpha=0.3)
ax2.legend(title="Matrices", loc='best', fontsize=10)

plt.tight_layout()
out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.savefig(out_path, dpi=300, bbox_inches='tight')
print(f"Successfully saved GFLOPS comparison to {out_path}")
