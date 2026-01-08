import pandas as pd
import matplotlib.pyplot as plt
import os
import matplotlib.ticker as ticker
import numpy as np

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)

# Read both CSV files
INPUT_FILE_REPLVEC = os.path.join(parent_dir, 'results', 'weak_scaling_replvec.csv')
INPUT_FILE_GHOST = os.path.join(parent_dir, 'results', 'weak_scaling_ghost.csv')
OUTPUT_DIR = os.path.join(parent_dir, 'plots')

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# Function to load and process data
def load_data(input_file, mode_name):
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
    df['Mode'] = mode_name
    
    return df, t1_total

# Function to create plots for a single mode
def create_plot(df, t1_total, mode_name, output_filename, color_main, color_ideal):
    fig, (ax1, ax2, ax3) = plt.subplots(1, 3, figsize=(24, 6))
    ticks_x = [1, 2, 4, 8, 16, 32, 64, 128]
    
    # ========== Plot 1: Total Execution Time with LOG SCALE ==========
    ax1.plot(df['Processes'], df['Total'], 
             f'{color_main}o-', linewidth=2, markersize=8, 
             label=f'{mode_name} (Measured)')
    ax1.axhline(y=t1_total, color=color_main, linestyle='--', linewidth=2, 
                label=f'Ideal ({t1_total:.2f} ms)', alpha=0.7)
    
    ax1.set_title(f'Weak Scaling: Total Execution Time ({mode_name})', 
                  fontsize=14, fontweight='bold')
    ax1.set_xlabel('Number of Processes', fontsize=12)
    ax1.set_ylabel('Total Time (ms) in Log Scale', fontsize=12)
    
    # LOG SCALE on both axes
    ax1.set_xscale('log')
    ax1.set_yscale('log')
    
    ax1.set_xticks(ticks_x)
    ax1.set_xticklabels(ticks_x)
    ax1.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax1.get_yaxis().set_major_formatter(ticker.ScalarFormatter())
    
    ax1.grid(True, which="both", ls="-", alpha=0.3)
    ax1.legend(loc='best')
    
    # ========== Plot 2: Efficiency ==========
    ax2.plot(df['Processes'], df['Efficiency'], 
             f'{color_main}o-', linewidth=2, markersize=8, 
             label=f'{mode_name} Efficiency')
    ax2.axhline(y=1.0, color='k', linestyle='--', linewidth=2, 
                label='Ideal (1.0)')
    
    ax2.set_title(f'Weak Scaling: Efficiency ({mode_name})', 
                  fontsize=14, fontweight='bold')
    ax2.set_xlabel('Number of Processes', fontsize=12)
    ax2.set_ylabel('Efficiency (T₁ / Tₚ)', fontsize=12)
    ax2.set_xscale('log')
    ax2.set_xticks(ticks_x)
    ax2.set_xticklabels(ticks_x)
    ax2.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    
    # Dynamic Y-axis limits for efficiency
    eff_min = df['Efficiency'].min()
    eff_max = df['Efficiency'].max()
    
    y_min_eff = max(0, min(eff_min * 0.9, 0.5))
    
    if eff_max > 1.0:
        y_max_eff = eff_max * 1.1
    else:
        y_max_eff = 1.05
    
    ax2.set_ylim(y_min_eff, y_max_eff)
    
    ax2.grid(True, which="both", ls="-", alpha=0.3)
    ax2.legend(loc='best')
    
    # ========== Plot 3: Time Breakdown (%) ==========
    procs = df['Processes'].astype(str).values
    x_pos = np.arange(len(procs))
    
    ax3.bar(x_pos, df['Pct_Comp'].values, 
            color='#2ECC71', label='Computation', 
            edgecolor='white', width=0.6)
    ax3.bar(x_pos, df['Pct_Comm'].values, 
            bottom=df['Pct_Comp'].values, 
            color='#E74C3C', label='Communication', 
            edgecolor='white', width=0.6)
    
    # Percentage labels
    for i in range(len(df)):
        pct_comp = df['Pct_Comp'].values[i]
        pct_comm = df['Pct_Comm'].values[i]
        
        if pct_comp > 5:
            ax3.text(i, pct_comp / 2, f'{pct_comp:.0f}%',
                    ha='center', va='center', fontsize=9,
                    fontweight='bold', color='black')
        if pct_comm > 5:
            ax3.text(i, pct_comp + pct_comm / 2, f'{pct_comm:.0f}%',
                    ha='center', va='center', fontsize=9,
                    fontweight='bold', color='white')
    
    ax3.set_title(f'Weak Scaling: Time Breakdown ({mode_name})', 
                  fontsize=14, fontweight='bold')
    ax3.set_xlabel('Number of Processes', fontsize=12)
    ax3.set_ylabel('Percentage of Total Time (%)', fontsize=12)
    ax3.set_xticks(x_pos)
    ax3.set_xticklabels(procs)
    ax3.set_ylim(0, 100)
    ax3.legend(loc='upper right')
    ax3.grid(axis='y', linestyle='--', alpha=0.3)
    ax3.set_axisbelow(True)
    
    plt.tight_layout()
    out_path = os.path.join(OUTPUT_DIR, output_filename)
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    print(f"Successfully saved plot to {out_path}")
    plt.close()

# Load both datasets
df_replvec, t1_replvec = load_data(INPUT_FILE_REPLVEC, 'Replicated Vector')
df_ghost, t1_ghost = load_data(INPUT_FILE_GHOST, 'Ghost Entries')

# Generate plots
if df_replvec is not None:
    create_plot(df_replvec, t1_replvec, 
                'Replicated Vector', 
                'weak_scaling_replvec.png', 
                'b',  # blue for replvec
                'b')
    
if df_ghost is not None:
    create_plot(df_ghost, t1_ghost, 
                'Ghost Entries', 
                'weak_scaling_ghost.png', 
                'r',  # red for ghost
                'r')

if df_replvec is None and df_ghost is None:
    print("CRITICAL ERROR: No data files found!")
    exit(1)

print("\nAll weak scaling plots generated successfully!")
