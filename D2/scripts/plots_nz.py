import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

INPUT_WEAK = os.path.join(parent_dir, 'results', 'weak_scaling.csv')
INPUT_STRONG = os.path.join(parent_dir, 'results', 'strong_scaling.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'load_balance_comparison.png'

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
df_weak['NZ_Min'] = pd.to_numeric(df_weak['NZ_Min'], errors='coerce')
df_weak['NZ_Max'] = pd.to_numeric(df_weak['NZ_Max'], errors='coerce')
df_weak['NZ_Avg'] = pd.to_numeric(df_weak['NZ_Avg'], errors='coerce')
df_weak = df_weak.dropna()

df_strong = pd.read_csv(INPUT_STRONG)
df_strong.columns = df_strong.columns.str.strip()
df_strong['Matrix'] = df_strong['Matrix'].str.strip()
df_strong['Processes'] = pd.to_numeric(df_strong['Processes'], errors='coerce')
df_strong['NZ_Min'] = pd.to_numeric(df_strong['NZ_Min'], errors='coerce')
df_strong['NZ_Max'] = pd.to_numeric(df_strong['NZ_Max'], errors='coerce')
df_strong['NZ_Avg'] = pd.to_numeric(df_strong['NZ_Avg'], errors='coerce')
df_strong = df_strong.dropna()

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(20, 7))
ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]

# Weak Scaling (Left)
yerr_lower = df_weak['NZ_Avg'] - df_weak['NZ_Min']
yerr_upper = df_weak['NZ_Max'] - df_weak['NZ_Avg']
yerr = [yerr_lower, yerr_upper]

ax1.errorbar(df_weak['Processes'], df_weak['NZ_Avg'],
             yerr=yerr, marker='o', linewidth=3,
             markersize=10, capsize=5, capthick=2,
             color='red', label='Weak Scaling', alpha=0.8)

ax1.set_title('Weak Scaling: Load Balance', fontsize=16, fontweight='bold')
ax1.set_xlabel('Number of Processes', fontsize=14)
ax1.set_ylabel('Non-zeros per Rank', fontsize=14)
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
        yerr_lower = subset['NZ_Avg'] - subset['NZ_Min']
        yerr_upper = subset['NZ_Max'] - subset['NZ_Avg']
        yerr = [yerr_lower, yerr_upper]
        ax2.errorbar(subset['Processes'], subset['NZ_Avg'],
                     yerr=yerr, marker='o', linewidth=2,
                     markersize=6, capsize=4, capthick=2,
                     label=mat, alpha=0.8)

ax2.set_title('Strong Scaling: Load Balance', fontsize=16, fontweight='bold')
ax2.set_xlabel('Number of Processes', fontsize=14)
ax2.set_ylabel('Non-zeros per Rank', fontsize=14)
ax2.set_xscale('log')
ax2.set_yscale('log')
ax2.set_xticks(ticks_x)
ax2.set_xticklabels(ticks_x)
ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
# Remove the y-axis formatter lines - let log scale use default
ax2.grid(True, which="both", ls="-", alpha=0.3)
ax2.legend(title="Matrices", loc='best', fontsize=11)



plt.tight_layout()
out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.savefig(out_path, dpi=300, bbox_inches='tight')
print(f"Successfully saved load balance comparison to {out_path}")
