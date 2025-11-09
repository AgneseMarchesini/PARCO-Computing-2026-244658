import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from pathlib import Path

# --- Configuration ---
RESULTS_DIR = Path("results")
PLOTS_DIR = Path("plots")
RESULTS_CSV = RESULTS_DIR / "benchmark_results.csv"
# --- End Configuration ---

def calculate_speedup(df):
    """Calculates speedup based on sequential time for each matrix."""
    # Find the single 'seq' time for each matrix
    seq_times = df[df["Mode"] == "seq"].set_index("Matrix")["Time_ms"]
    
    # Map this 'seq_time' to every row in the dataframe
    df["Seq_Time"] = df["Matrix"].map(seq_times)
    
    # Speedup = T_sequential / T_parallel
    df["Speedup"] = df["Seq_Time"] / df["Time_ms"]
    return df

def plot_speedup_vs_threads(df, plots_dir):
    """
    Plots Speedup vs. Threads for each matrix and chunk size.
    This is the most important plot for your report.
    """
    print("Generating Speedup vs. Threads plots...")
    
    # We don't plot 'seq' here, as speedup is the focus
    parallel_df = df[df["Mode"] != "seq"].copy()
    
    # Get a list of all unique matrices
    matrices = parallel_df["Matrix"].unique()
    
    for matrix in matrices:
        matrix_df = parallel_df[parallel_df["Matrix"] == matrix]
        
        # Use seaborn.relplot to create a faceted line plot
        # col="ChunkSize" creates a new subplot for each chunk size
        g = sns.relplot(
            data=matrix_df,
            x="Threads",
            y="Speedup",
            hue="Mode",         # Different color lines for static, dynamic, guided
            style="Mode",       # Different markers for each mode
            col="ChunkSize",    # New plot for each chunk size
            kind="line",
            marker="o",
            col_wrap=3          # Wrap to a new row after 3 subplots
        )
        
        # Set titles and layout
        g.fig.suptitle(f"Speedup vs. Threads for {matrix}", y=1.03)
        g.set(xscale='log', xticks=[1, 2, 4, 8, 16, 32, 64])
        g.set_axis_labels("Threads (log scale)", "Speedup")
        
        # Save the plot
        plot_filename = plots_dir / f"speedup_{matrix.split('.')[0]}.png"
        g.savefig(plot_filename)
        plt.close(g.fig)

def plot_time_vs_threads(df, plots_dir):
    """
    Plots raw Time (ms) vs. Threads.
    Useful for seeing the memory bandwidth saturation.
    """
    print("Generating Time vs. Threads plots...")
    
    matrices = df["Matrix"].unique()
    
    for matrix in matrices:
        matrix_df = df[df["Matrix"] == matrix]
        
        g = sns.relplot(
            data=matrix_df,
            x="Threads",
            y="Time_ms",
            hue="Mode",
            style="Mode",
            col="ChunkSize",
            kind="line",
            marker="o",
            col_wrap=3
        )
        
        # Use log scales for both axes
        g.set(xscale='log', yscale='log', 
              xticks=[1, 2, 4, 8, 16, 32, 64],
              xticklabels=[1, 2, 4, 8, 16, 32, 64])
        g.fig.suptitle(f"Execution Time vs. Threads for {matrix}", y=1.03)
        g.set_axis_labels("Threads (log scale)", "Time (ms) (log scale)")
        
        plot_filename = plots_dir / f"time_vs_threads_{matrix.split('.')[0]}.png"
        g.savefig(plot_filename)
        plt.close(g.fig)

def plot_chunk_analysis(df, plots_dir):
    """
    Plots Time (ms) vs. ChunkSize.
    Useful for finding the optimal chunk size.
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
            col="Threads",      # New plot for each thread count
            kind="line",
            marker="o",
            col_wrap=4
        )
        
        # Use log scales
        g.set(xscale='log', yscale='log')
        g.fig.suptitle(f"Chunk Size Analysis for {matrix}", y=1.03)
        g.set_axis_labels("Chunk Size (log scale)", "Time (ms) (log scale)")
        
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

    # --- Data Cleaning ---
    # Convert ChunkSize to numeric, forcing errors (like "NA") into NaN
    df["ChunkSize"] = pd.to_numeric(df["ChunkSize"], errors='coerce')

    # Fill all NaN values (which includes the 'seq' rows) with 1
    df["ChunkSize"] = df["ChunkSize"].fillna(1)

    # Now it is safe to convert to integer
    df["ChunkSize"] = df["ChunkSize"].astype(int)
        
    # Replace any 0.0 times with a very small number to avoid errors in log scales
    df["Time_ms"] = df["Time_ms"].replace(0, 1e-6)
    
    # --- Calculate Speedup ---
    df = calculate_speedup(df)
    
    # --- Generate Plots ---
    plot_speedup_vs_threads(df, PLOTS_DIR)
    plot_time_vs_threads(df, PLOTS_DIR)
    plot_chunk_analysis(df, PLOTS_DIR)
    
    print("\nAll plots generated successfully")
    print(f"Plots are saved in the '{PLOTS_DIR}' directory.")

if __name__ == "__main__":
    main()