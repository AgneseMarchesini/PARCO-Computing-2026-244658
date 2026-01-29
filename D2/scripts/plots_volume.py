import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

INPUT_WEAK = os.path.join(parent_dir, 'results', 'weak_scaling.csv')
INPUT_STRONG = os.path.join(parent_dir, 'results', 'strong_scaling.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'communication_volume_comparison.png'

MATRIX_ORDER = [
    'venkat25.mtx',
    'atmosmodl.mtx',
    'rajat31.mtx',
    'circuit5M.mtx',
    'cage15.mtx'
]

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

df_weak = pd.read_csv(INPUT_WEAK)
df_weak.columns = df_weak.columns.str.strip()
df_weak['Processes'] = pd.to_numeric(df_weak['Processes'], errors='coerce')
df_weak['Volume_MB'] = pd.to_numeric(df_weak['Volume_MB'], errors='coerce')
df_weak = df_weak.dropna()

df_strong = pd.read_csv(INPUT_STRONG)
df_strong.columns = df_strong.columns.str.strip()
df_strong['Matrix'] = df_strong['Matrix'].str.strip()
df_strong['Processes'] = pd.to_numeric(df_strong['Processes'], errors='coerce')
df_strong['Volume_MB'] = pd.to_numeric(df_strong['Volume_MB'], errors='coerce')
df_strong = df_strong.dropna()

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 7))
ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]

# Weak Scaling (Left)
ax1.plot(df_weak['Processes'], df_weak['Volume_MB'],
         'ro-', linewidth=3, markersize=10, label='Weak Scaling')
ax1.set_title('Weak Scaling', fontsize=16, fontweight='bold')
ax1.set_xlabel('Number of Processes', fontsize=14)
ax1.set_ylabel('Communication Volume (MB/rank)', fontsize=14)
ax1.set_xscale('log')
ax1.set_xticks(ticks_x)
ax1.set_xticklabels(ticks_x)
ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax1.grid(True, which="both", ls="-", alpha=0.3)
ax1.legend(loc='best', fontsize=12)

# Strong Scaling (Right)
for mat in MATRIX_ORDER:
    if mat in df_strong['Matrix'].unique():
        subset = df_strong[df_strong['Matrix'] == mat].sort_values('Processes')
        ax2.plot(subset['Processes'], subset['Volume_MB'],
                 marker='o', linewidth=2, markersize=6, label=mat)

ax2.set_title('Strong Scaling', fontsize=16, fontweight='bold')
ax2.set_xlabel('Number of Processes', fontsize=14)
ax2.set_ylabel('Communication Volume (MB/rank)', fontsize=14)
ax2.set_xscale('log')
ax2.set_yscale('log')
ax2.set_xticks(ticks_x)
ax2.set_xticklabels(ticks_x)
ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax2.get_yaxis().set_major_formatter(ticker.ScalarFormatter())
ax2.grid(True, which="both", ls="-", alpha=0.3)
ax2.legend(title="Matrices", loc='best', fontsize=11)

plt.tight_layout()
out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.savefig(out_path, dpi=300, bbox_inches='tight')
print(f"Successfully saved volume comparison to {out_path}")
