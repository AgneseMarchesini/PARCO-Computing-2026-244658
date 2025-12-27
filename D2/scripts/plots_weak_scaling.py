#  python3 .\D2\scripts\plots_weak_scaling.py

import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker 

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
df['Time'] = pd.to_numeric(df['Time'], errors='coerce')
df = df.dropna().sort_values('Processes')

t1 = df[df['Processes'] == 1]['Time'].values[0]
df['Efficiency'] = t1 / df['Time']

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))

ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]

ax1.plot(df['Processes'], df['Time'], 'bo-', linewidth=2, markersize=8, label='Measured Time')
ax1.axhline(y=t1, color='k', linestyle='--', label='Ideal (Constant)')

ax1.set_title('Weak Scaling: Execution Time', fontsize=14)
ax1.set_xlabel('Number of Processes', fontsize=12)
ax1.set_ylabel('Execution Time (ms)', fontsize=12)
ax1.set_xscale('log')
ax1.set_xticks(ticks_x)
ax1.set_xticklabels(ticks_x)
max_time = df['Time'].max()
ax1.set_ylim(0, max_time * 1.5)
ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax1.grid(True, which="both", ls="-", alpha=0.5)
ax1.legend()

ax2.plot(df['Processes'], df['Efficiency'], 'go-', linewidth=2, markersize=8, label='Efficiency')
ax2.axhline(y=1.0, color='k', linestyle='--', label='Ideal (1.0)')

ax2.set_title('Weak Scaling: Efficiency', fontsize=14)
ax2.set_xlabel('Number of Processes', fontsize=12)
ax2.set_ylabel('Efficiency (Normalized to P=1)', fontsize=12)
ax2.set_xscale('log')
ax2.set_xticks(ticks_x)
ax2.set_xticklabels(ticks_x)
ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
ax2.set_ylim(0, max(1.5, df['Efficiency'].max() + 0.1))
ax2.grid(True, which="both", ls="-", alpha=0.5)
ax2.legend()

out_path = os.path.join(OUTPUT_DIR, OUTPUT_FILE)
plt.tight_layout()
plt.savefig(out_path)
print(f"Successfully saved plot to {out_path}")