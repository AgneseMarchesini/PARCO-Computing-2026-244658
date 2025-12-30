#  python3 .\D2\scripts\plots_weak_scaling.py
import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker
import numpy as np

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

INPUT_FILE = os.path.join(parent_dir, 'results', 'weak_scaling.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')
OUTPUT_FILE = 'weak_scaling.png'

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

print(f"Reading {INPUT_FILE}...")

try:
    df = pd.read_csv(INPUT_FILE)
    if len(df.columns) < 3:
        df = pd.read_csv(INPUT_FILE, delimiter=';')
except FileNotFoundError:
    print(f"CRITICAL ERROR: Could not find file at: {INPUT_FILE}")
    exit(1)
except Exception as e:
    print(f"Error reading CSV: {e}")
    exit(1)

df.columns = df.columns.str.strip()
df['Processes'] = pd.to_numeric(df['Processes'], errors='coerce')
df['T_Comp'] = pd.to_numeric(df['T_Comp'], errors='coerce')
df['T_Comm'] = pd.to_numeric(df['T_Comm'], errors='coerce')

df = df.dropna().sort_values('Processes')

df['Total'] = df['T_Comp'] + df['T_Comm']
df['Pct_Comp'] = (df['T_Comp'] / df['Total']) * 100
df['Pct_Comm'] = (df['T_Comm'] / df['Total']) * 100

t1_comp = df[df['Processes'] == 1]['T_Comp'].values[0]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(18, 6))

ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]

ax1.plot(
    df['Processes'],
    df['T_Comp'],
    'bo-',
    linewidth=2,
    markersize=8,
    label='Measured Computation'
)

ax1.axhline(
    y=t1_comp,
    color='k',
    linestyle='--',
    linewidth=2,
    label='Ideal (Constant)'
)

ax1.set_title('Weak Scaling: Computation Time', fontsize=14)
ax1.set_xlabel('Number of Processes', fontsize=12)
ax1.set_ylabel('Execution Time (ms)', fontsize=12)

ax1.set_xscale('log')
ax1.set_xticks(ticks_x)
ax1.set_xticklabels(ticks_x)
ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())

ax1.set_ylim(0, df['T_Comp'].max() * 1.5)
ax1.get_yaxis().set_major_formatter(ticker.ScalarFormatter())

ax1.grid(True, which="both", ls="-", alpha=0.5)
ax1.legend()

procs = df['Processes'].astype(str).values
x_pos = np.arange(len(procs))

ax2.bar(
    x_pos,
    df['Pct_Comp'].values,
    color='green',
    label='Computation',
    edgecolor='white',
    width=0.6
)

ax2.bar(
    x_pos,
    df['Pct_Comm'].values,
    bottom=df['Pct_Comp'].values,
    color='red',
    label='Communication',
    edgecolor='white',
    width=0.6
)

ax2.set_title('Weak Scaling: Time Breakdown (%)', fontsize=14)
ax2.set_xlabel('Number of Processes', fontsize=12)
ax2.set_ylabel('Percentage of Total Time (%)', fontsize=12)

ax2.set_xticks(x_pos)
ax2.set_xticklabels(procs)

ax2.set_ylim(0, 100)

# Percentage labels
for i in range(len(df)):
    pct_comp = df['Pct_Comp'].values[i]
    pct_comm = df['Pct_Comm'].values[i]

    if pct_comp > 5:
        ax2.text(
            i,
            pct_comp / 2,
            f'{pct_comp:.0f}%',
            ha='center',
            va='center',
            fontsize=9,
            fontweight='bold',
            color='black'
        )

    if pct_comm > 5:
        ax2.text(
            i,
            pct_comp + pct_comm / 2,
            f'{pct_comm:.0f}%',
            ha='center',
            va='center',
            fontsize=9,
            fontweight='bold',
            color='white'
        )

ax2.legend(loc='upper right')
ax2.grid(axis='y', linestyle='--', alpha=0.5)
ax2.set_axisbelow(True)

plt.tight_layout()
out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.savefig(out_path)
print(f"Successfully saved plot to {out_path}")
