import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker
import numpy as np


script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)


# Read CSV file
INPUT_FILE = os.path.join(parent_dir, 'results', 'weak_scaling.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')


if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)


# Function to load and process data
def load_data(input_file):
    print(f"Reading {input_file}...")
    try:
        df = pd.read_csv(input_file)
        if len(df.columns) < 3:
            df = pd.read_csv(input_file, delimiter=';')
    except FileNotFoundError:
        print(f"WARNING: Could not find file at: {input_file}")
        return None, None
    except Exception as e:
        print(f"Error reading CSV: {e}")
        return None, None


    df.columns = df.columns.str.strip()
    df['Processes'] = pd.to_numeric(df['Processes'], errors='coerce')
    df['T_Comp'] = pd.to_numeric(df['T_Comp'], errors='coerce')
    df['T_Comm'] = pd.to_numeric(df['T_Comm'], errors='coerce')
    df = df.dropna().sort_values('Processes')


    # Use Total Time for weak scaling
    df['Total'] = df['T_Comp'] + df['T_Comm']
    df['Pct_Comp'] = (df['T_Comp'] / df['Total']) * 100
    df['Pct_Comm'] = (df['T_Comm'] / df['Total']) * 100


    # Weak Scaling Efficiency = T1_total / Tp_total
    t1_total = df[df['Processes'] == 1]['Total'].values[0]
    df['Efficiency'] = t1_total / df['Total']


    return df, t1_total


# Function to create 1x4 weak scaling figure
def create_weak_scaling_plots(df, t1_total, output_filename, color_main='r'):
    # Filter for linear plot (no 128P)
    df_linear = df[df['Processes'] < 128].copy()
    
    fig, (ax1, ax2, ax3, ax4) = plt.subplots(1, 4, figsize=(28, 6))
    ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]
    ticks_x_linear = [1, 2, 4, 8, 16, 32, 64]


    # ========== Plot 1: Total Time NORMAL SCALE (all processes) ==========
    ax1.plot(df['Processes'], df['Total'],
             f'{color_main}o-', linewidth=2, markersize=8,
             label='Measured')
    ax1.axhline(y=t1_total, color=color_main, linestyle='--', linewidth=2,
                label=f'Ideal ({t1_total:.2f} ms)', alpha=0.7)
    ax1.set_title('Total Time (Normal Scale)', fontsize=12, fontweight='bold')
    ax1.set_xlabel('Processes')
    ax1.set_ylabel('Time (ms)')
    ax1.set_xscale('log')
    ax1.set_xticks(ticks_x)
    ax1.set_xticklabels(ticks_x)
    ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax1.grid(True, ls="-", alpha=0.3)
    ax1.legend(loc='best', fontsize=10)


    # ========== Plot 2: Total Time LINEAR SCALE (≤64P) ==========
    ax2.plot(df_linear['Processes'], df_linear['Total'],
             f'{color_main}o-', linewidth=2, markersize=8,
             label='Measured')
    ax2.axhline(y=t1_total, color=color_main, linestyle='--', linewidth=2,
                label=f'Ideal ({t1_total:.2f} ms)', alpha=0.7)
    ax2.set_title('Total Time (Linear, ≤64P)', fontsize=12, fontweight='bold')
    ax2.set_xlabel('Processes')
    ax2.set_ylabel('Time (ms)')
    ax2.set_xscale('log')
    ax2.set_xticks(ticks_x_linear)
    ax2.set_xticklabels(ticks_x_linear)
    ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax2.set_ylim(-0.02, 0.2)
    ax2.grid(True, ls="-", alpha=0.3)
    ax2.legend(loc='best', fontsize=10)


    # ========== Plot 3: Efficiency ==========
    ax3.plot(df['Processes'], df['Efficiency'],
             f'{color_main}o-', linewidth=2, markersize=8,
             label='Efficiency')
    ax3.axhline(y=1.0, color='k', linestyle='--', linewidth=2,
                label='Ideal (1.0)')
    ax3.set_title('Efficiency', fontsize=12, fontweight='bold')
    ax3.set_xlabel('Processes')
    ax3.set_ylabel('T₁ / Tₚ')
    ax3.set_xscale('log')
    ax3.set_xticks(ticks_x)
    ax3.set_xticklabels(ticks_x)
    ax3.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax3.grid(True, which="both", ls="-", alpha=0.3)
    ax3.legend(loc='best', fontsize=10)


    # ========== Plot 4: Time Breakdown (%) ==========
    procs = df['Processes'].astype(str).values
    x_pos = np.arange(len(procs))
    ax4.bar(x_pos, df['Pct_Comp'].values,
            color='#2ECC71', label='Compute',
            edgecolor='white', width=0.6)
    ax4.bar(x_pos, df['Pct_Comm'].values,
            bottom=df['Pct_Comp'].values,
            color='#E74C3C', label='Comm',
            edgecolor='white', width=0.6)


    for i in range(len(df)):
        pct_comp = df['Pct_Comp'].values[i]
        pct_comm = df['Pct_Comm'].values[i]
        if pct_comp > 10:
            ax4.text(i, pct_comp / 2, f'{pct_comp:.0f}%',
                     ha='center', va='center', fontsize=8,
                     fontweight='bold', color='black')
        if pct_comm > 10:
            ax4.text(i, pct_comp + pct_comm / 2, f'{pct_comm:.0f}%',
                     ha='center', va='center', fontsize=8,
                     fontweight='bold', color='white')


    ax4.set_title('Time Breakdown (%)', fontsize=12, fontweight='bold')
    ax4.set_xlabel('Processes')
    ax4.set_ylabel('% Total Time')
    ax4.set_xticks(x_pos)
    ax4.set_xticklabels(procs, rotation=45, ha='right')
    ax4.set_ylim(0, 100)
    ax4.legend(loc='upper right', fontsize=10)
    ax4.grid(axis='y', linestyle='--', alpha=0.3)
    ax4.set_axisbelow(True)


    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, output_filename)
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    print(f"Successfully saved plot to {out_path}")
    plt.close()


# Load data and generate plot
df, t1_total = load_data(INPUT_FILE)
if df is not None:
    create_weak_scaling_plots(df, t1_total, 'weak_scaling.png', 'r')
else:
    print("CRITICAL ERROR: No data file found!")
    exit(1)


print("\nWeak scaling plot generated successfully!")
