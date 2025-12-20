import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from pathlib import Path
import matplotlib.ticker as ticker 

# --- Configuration (Copied from your script) ---
RESULTS_DIR = Path("results")
PLOTS_DIR = Path("plots")
RESULTS_CSV = RESULTS_DIR / "time_results.csv"
CHUNK_SIZES_TO_PLOT = [1, 10, 100, 1000] 
THREADS_TO_PLOT = [1, 2, 4, 8, 16, 32, 64] # Will filter out 1 thread for parallel plots
# --- End Configuration ---

def calculate_speedup(df):
    """Calculates speedup based on sequential time for each matrix."""
    print("Calculating speedup...")
    # Get sequential times (where Mode is 'seq')
    seq_times = df[df["Mode"] == "seq"].set_index("Matrix")["Time_ms"]
    
    # Map sequential time to all rows
    df["Seq_Time"] = df["Matrix"].map(seq_times)
    
    # Handle cases where a matrix might be missing a 'seq' run (though it shouldn't)
    df["Seq_Time"] = df["Seq_Time"].fillna(df["Time_ms"]) 
    
    df["Speedup"] = df["Seq_Time"] / df["Time_ms"]
    
    # For 'seq' mode itself, speedup is 1
    df.loc[df["Mode"] == "seq", "Speedup"] = 1.0
    return df

def plot_speedup_vs_threads_bars_combined(df, plots_dir):
    """
    Plots Speedup vs. Threads as a grouped bar chart.
    Generates one plot per Matrix, with subplots for each Mode.
    """
    print("Generating Speedup vs. Threads combined bar plots...")
    
    # Filter for parallel modes and specified chunk/thread counts
    parallel_df = df[df["Mode"] != "seq"].copy()
    
    # Filter using the config lists
    parallel_df = parallel_df[parallel_df['ChunkSize'].isin(CHUNK_SIZES_TO_PLOT)]
    # Filter out 1 thread from parallel modes, as speedup is vs. 1 thread
    parallel_df = parallel_df[parallel_df['Threads'].isin(THREADS_TO_PLOT)]
    parallel_df = parallel_df[parallel_df['Threads'] > 1] 

    if parallel_df.empty:
        print("No parallel data found matching filters. No plots will be generated.")
        return

    # Convert ChunkSize to string/category so seaborn treats it discretely
    parallel_df["ChunkSize"] = parallel_df["ChunkSize"].astype(str)

    matrices = parallel_df["Matrix"].unique()
    
    for matrix in matrices:
        # Filter data for the current matrix
        plot_df = parallel_df[parallel_df["Matrix"] == matrix]
        
        if plot_df.empty:
            print(f"  Skipping plot for {matrix} - No data.")
            continue
            
        print(f"  Generating plot for {matrix} (all modes)...")

        # Create the faceted bar chart using catplot
        # 'col="Mode"' creates the three subplots
        g = sns.catplot(
            data=plot_df,
            x="Threads",
            y="Speedup",
            hue="ChunkSize",  # Group bars by ChunkSize
            col="Mode",       # Create subplots for each Mode
            kind="bar",
            height=5,         # Height of each subplot
            aspect=1.1,       # Aspect ratio of each subplot
            legend_out=True
        )
        
        # Iterate over each subplot's axes
        for ax in g.axes.flat:
            # Add a horizontal line at y=1 (sequential performance)
            ax.axhline(1.0, ls='--', color='red', linewidth=0.8)
            ax.set_xticklabels(ax.get_xticklabels(), rotation=0)

        # Set titles and labels
        g.fig.suptitle(f"Speedup vs. Threads for {matrix}", y=1.03)
        g.set_axis_labels("Number of Threads", "Speedup")
        g.set_titles("Mode = {col_name}") # Titles for each subplot
        
        # Save the plot
        # Filename is now just per-matrix
        plot_filename = plots_dir / f"speedup_bars_combined_{matrix.split('.')[0]}.png"
        g.savefig(plot_filename, bbox_inches='tight')
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
        print("Please ensure 'time_results.csv' is in the 'results' directory.")
        return

    # --- Data Cleaning (Robust Version from your script) ---
    print("Cleaning data...")
    df["ChunkSize"] = pd.to_numeric(df["ChunkSize"], errors='coerce')
    df["ChunkSize"] = df["ChunkSize"].fillna(1) # Use 1 for 'seq' mode
    df["ChunkSize"] = df["ChunkSize"].astype(int)
    
    # Prevent division by zero if time is 0
    df["Time_ms"] = df["Time_ms"].replace(0, 1e-6) 
    
    # --- Calculate Speedup ---
    df = calculate_speedup(df)
    
    # --- Generate Plots ---
    plot_speedup_vs_threads_bars_combined(df, PLOTS_DIR)
    
    print("\nPlot generation complete.")
    print(f"Plots are saved in the '{PLOTS_DIR}' directory.")

if __name__ == "__main__":
    main()