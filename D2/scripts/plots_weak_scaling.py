import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker
import numpy as np

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

INPUT_FILE = os.path.join(parent_dir, 'results', 'weak_scaling.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')

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
        return None, None
    except Exception as e:
        print(f"Error reading CSV: {e}")
        return None, None

    df.columns = df.columns.str.strip()
    df['Processes'] = pd.to_numeric(df['Processes'], errors='coerce')
    df['T_Comp'] = pd.to_numeric(df['T_Comp'], errors='coerce')
    df['T_Comm'] = pd.to_numeric(df['T_Comm'], errors='coerce')
    df = df.dropna().sort_values('Processes')

    df['Total'] = df['T_Comp'] + df['T_Comm']
    df['Pct_Comp'] = (df['T_Comp'] / df['Total']) * 100
    df['Pct_Comm'] = (df['T_Comm'] / df['Total']) * 100

    t1_total = df[df['Processes'] == 1]['Total'].values[0]
    df['Efficiency'] = t1_total / df['Total']

    return df, t1_total

def create_weak_scaling_plots(df, t1_total, output_filename, color_main='r'):
    df_linear = df[df['Processes'] < 128].copy()
    
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(21, 6))
    ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]
    ticks_x_linear = [1, 2, 4, 8, 16, 32, 64]

    # ========== Plot 1: Total Time NORMAL SCALE ==========
    ax1.plot(df['Processes'], df['Total'],
             f'{color_main}o-', linewidth=2, markersize=8)
    ax1.set_title('Total Time', fontsize=12, fontweight='bold')
    ax1.set_xlabel('Processes')
    ax1.set_ylabel('Time (ms)')
    ax1.set_xscale('log')
    ax1.set_xticks(ticks_x)
    ax1.set_xticklabels(ticks_x)
    ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax1.grid(True, ls="-", alpha=0.3)

    # ========== Plot 2: Efficiency ==========
    ax2.plot(df['Processes'], df['Efficiency'],
             f'{color_main}o-', linewidth=2, markersize=8)
    ax2.set_title('Efficiency', fontsize=12, fontweight='bold')
    ax2.set_xlabel('Processes')
    ax2.set_ylabel('T₁ / Tₚ')
    ax2.set_xscale('log')
    ax2.set_xticks(ticks_x)
    ax2.set_xticklabels(ticks_x)
    ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax2.grid(True, which="both", ls="-", alpha=0.3)

    # ========== Plot 3: Time Breakdown (ms, ≤64P) ==========
    procs = df_linear['Processes'].astype(str).values
    x_pos = np.arange(len(procs))

    ax3.bar(x_pos, df_linear['T_Comp'].values,
            color='#2ECC71', label='Computation',
            edgecolor='white', width=0.6)
    ax3.bar(x_pos, df_linear['T_Comm'].values,
            bottom=df_linear['T_Comp'].values,
            color='#E74C3C', label='Communication',
            edgecolor='white', width=0.6)

    # Add percentage annotations
    for i in range(len(df_linear)):
        pct_comp = df_linear['Pct_Comp'].values[i]
        pct_comm = df_linear['Pct_Comm'].values[i]
        t_comp = df_linear['T_Comp'].values[i]
        t_comm = df_linear['T_Comm'].values[i]
        t_total = t_comp + t_comm
        
        # Show compute percentage if >10%
        if pct_comp > 10:
            ax3.text(i, t_comp / 2, f'{pct_comp:.0f}%',
                    ha='center', va='center', fontsize=8,
                    fontweight='bold', color='black')
        
        # Show comm percentage (always if >= 1%)
        if pct_comm >= 1:
            # Position above bar if too thin to fit text inside
            if t_comm < 0.004:  # If comm bar is very small
                ax3.text(i, t_total + 0.004, f'{pct_comm:.0f}%',
                        ha='center', va='bottom', fontsize=7,
                        fontweight='bold', color='#E74C3C')
            else:
                ax3.text(i, t_comp + t_comm / 2, f'{pct_comm:.0f}%',
                        ha='center', va='center', fontsize=8,
                        fontweight='bold', color='white')

    ax3.set_title('Time Breakdown (≤64P)', fontsize=12, fontweight='bold')
    ax3.set_xlabel('Processes')
    ax3.set_ylabel('Time (ms)')
    ax3.set_ylim(-0.01, 0.10)  
    ax3.set_xticks(x_pos)
    ax3.set_xticklabels(procs, rotation=45, ha='right')
    ax3.legend(loc='upper left', fontsize=10)
    ax3.grid(axis='y', linestyle='--', alpha=0.3)
    ax3.set_axisbelow(True)



    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, output_filename)
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    print(f"Successfully saved plot to {out_path}")
    plt.close()

df, t1_total = load_data(INPUT_FILE)
if df is not None:
    create_weak_scaling_plots(df, t1_total, 'weak_scaling.png', 'r')
else:
    print("CRITICAL ERROR: No data file found!")
    exit(1)

print("\nWeak scaling plot generated successfully!")
