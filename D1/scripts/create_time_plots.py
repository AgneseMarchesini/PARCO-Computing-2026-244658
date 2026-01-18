import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from pathlib import Path
import matplotlib.ticker as ticker 

# --- Configuration ---
RESULTS_DIR = Path("D1/results")
PLOTS_DIR = Path("D1/plots")
RESULTS_CSV = RESULTS_DIR / "time_results.csv"
CHUNK_SIZES_TO_PLOT = [1, 10, 100, 1000] 
THREADS_TO_PLOT = [1, 2, 4, 8, 16, 32, 64]
# --- End Configuration ---

def calculate_speedup(df):
    """Calculates speedup based on sequential time for each matrix."""
    print("Calculating speedup...")
    seq_times = df[df["Mode"] == "seq"].set_index("Matrix")["Time_ms"]
    df["Seq_Time"] = df["Matrix"].map(seq_times)
    df["Speedup"] = df["Seq_Time"] / df["Time_ms"]
    return df

def plot_speedup_vs_threads(df, plots_dir):
    """
    Plots Speedup vs. Threads.
    """
    print("Generating Speedup vs. Threads plots...")
    
    parallel_df = df[df["Mode"] != "seq"].copy()
    matrices = parallel_df["Matrix"].unique()
    
    for matrix in matrices:
        matrix_df = parallel_df[parallel_df["Matrix"] == matrix]
        
        g = sns.relplot(
            data=matrix_df,
            x="Threads",
            y="Speedup",
            hue="Mode",
            style="Mode",
            col="ChunkSize",
            kind="line",
            marker="o",
            height=5,
            aspect=1.2
        )
        
        for ax in g.axes.flat:
            ax.set_xscale('log')
            ax.set_xticks(THREADS_TO_PLOT)
            ax.set_xticklabels([str(t) for t in THREADS_TO_PLOT])
            ax.set_xlabel("Threads (log scale)")
            ax.set_ylabel("Speedup (T_seq / T_par)")
            ax.grid(which='major', axis='both', linestyle='--', linewidth=0.5)
            ax.grid(which='minor', axis='x', linestyle=':', linewidth=0.5)
        
        g.fig.suptitle(f"Speedup vs. Threads for {matrix}", y=1.05)
        g.set_titles("ChunkSize = {col_name}")
        
        plot_filename = plots_dir / f"speedup_{matrix.split('.')[0]}.png"
        g.savefig(plot_filename)
        plt.close(g.fig)

def plot_time_vs_threads(df, plots_dir):
    """
    Plots raw Time (ms) vs. Threads.
    NEW: Excludes 'seq' mode from this plot.
    """
    print("Generating Time vs. Threads plots...")
    
    matrices = df["Matrix"].unique()
    
    for matrix in matrices:
        
        # --- THIS IS THE FIX ---
        # Filter out the 'seq' mode before plotting
        matrix_df = df[(df["Matrix"] == matrix) & (df["Mode"] != "seq")].copy()
        # --- END FIX ---
        
        g = sns.relplot(
            data=matrix_df,
            x="Threads",
            y="Time_ms",
            hue="Mode",
            style="Mode",
            col="ChunkSize",
            kind="line",
            marker="o",
            height=5,
            aspect=1.2
        )
        
        for ax in g.axes.flat:
            ax.set_xscale('log')
            ax.set_yscale('log')
            ax.set_xticks(THREADS_TO_PLOT)
            ax.set_xticklabels([str(t) for t in THREADS_TO_PLOT])
            ax.set_xlabel("Threads (log scale)")
            ax.set_ylabel("Time (ms) (log scale)")
            ax.grid(which='major', axis='both', linestyle='--', linewidth=0.5)
            ax.grid(which='minor', axis='both', linestyle=':', linewidth=0.5)
            ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y, _: f'{y:g}'))

        g.fig.suptitle(f"Execution Time vs. Threads for {matrix}", y=1.05)
        g.set_titles("ChunkSize = {col_name}")
        
        plot_filename = plots_dir / f"time_vs_threads_{matrix.split('.')[0]}.png"
        g.savefig(plot_filename)
        plt.close(g.fig)

def plot_chunk_analysis(df, plots_dir):
    """
    Plots Time (ms) vs. ChunkSize.
    """
    print("Generating Chunk Size Analysis plots...")
    
    parallel_df = df[df["Mode"] != "seq"].copy()
    matrices = parallel_df["Matrix"].unique()
    
    for matrix in matrices:
        matrix_df = parallel_df[parallel_df["Matrix"] == matrix]
        
        g = sns.relplot(
            data=matrix_df,
            x="ChunkSize",
            y="Time_ms",
            hue="Mode",
            style="Mode",
            col="Threads",
            kind="line",
            marker="o",
            height=5,
            aspect=1.2
        )
        
        for ax in g.axes.flat:
            ax.set_xscale('log')
            ax.set_yscale('log')
            ax.set_xticks(CHUNK_SIZES_TO_PLOT)
            ax.set_xticklabels([str(t) for t in CHUNK_SIZES_TO_PLOT])
            ax.set_xlabel("Chunk Size (log scale)")
            ax.set_ylabel("Time (ms) (log scale)")
            ax.grid(which='major', axis='both', linestyle='--', linewidth=0.5)
            ax.grid(which='minor', axis='both', linestyle=':', linewidth=0.5)
            ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y, _: f'{y:g}'))

        g.fig.suptitle(f"Chunk Size Analysis for {matrix}", y=1.05)
        g.set_titles("Threads = {col_name}")
        
        plot_filename = plots_dir / f"chunk_analysis_{matrix.split('.')[0]}.png"
        g.savefig(plot_filename)
        plt.close(g.fig)

def main():
    # Create the 'plots' directory if it doesn't exist
    PLOTS_DIR.mkdir(exist_ok=True)
    
    # Load the data
    print(f"Loading data from {RESULTS_CSV}...")
    try:
        df = pd.read_csv(RESULTS_CSV)
    except FileNotFoundError:
        print(f"Error: {RESULTS_CSV} not found.")
        print("Please run your benchmark script first to generate the results.")
        return

    # --- Data Cleaning (Robust Version) ---
    print("Cleaning data...")
    df["ChunkSize"] = pd.to_numeric(df["ChunkSize"], errors='coerce')
    df["ChunkSize"] = df["ChunkSize"].fillna(1)
    df["ChunkSize"] = df["ChunkSize"].astype(int)
    df["Time_ms"] = df["Time_ms"].replace(0, 1e-6)
    
    # --- Calculate Speedup ---
    df = calculate_speedup(df)
    
    # --- Generate Plots ---
    plot_speedup_vs_threads(df, PLOTS_DIR)
    plot_time_vs_threads(df, PLOTS_DIR)
    plot_chunk_analysis(df, PLOTS_DIR)
    
    print("\nAll plots generated successfully!")
    print(f"Your plots are saved in the '{PLOTS_DIR}' directory.")

if __name__ == "__main__":
    main()