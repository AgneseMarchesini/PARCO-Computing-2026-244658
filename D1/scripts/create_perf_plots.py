import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.ticker import PercentFormatter, ScalarFormatter
import os

def plot_perf_data(csv_file_path, output_dir, output_prefix='plot'):
    """
    Loads a single performance CSV file, calculates cache miss ratios,
    and generates separate PNG plots for each matrix, including a
    sequential baseline on each plot.

    Args:
        csv_file_path (str): The full file path to the CSV file.
        output_dir (str): The directory where plots will be saved.
        output_prefix (str): A prefix for the saved plot filenames.
    """
    
    # Create the output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Load the dataset
    try:
        df = pd.read_csv(csv_file_path)
    except FileNotFoundError:
        print(f"Error: File not found at {csv_file_path}")
        return
    except Exception as e:
        print(f"An error occurred during file loading: {e}")
        return

    # Filter out rows with 0 cache references
    df_filtered = df[df['CacheRefs'] > 0].copy()
    
    if df_filtered.empty:
        print("No valid performance data found after filtering. No plots will be generated.")
        return

    # Calculate ratios
    epsilon = 1e-9
    df_filtered['CacheMissRatio'] = df_filtered['CacheMisses'] / (df_filtered['CacheRefs'] + epsilon)
    df_filtered['LLCMissRatio'] = df_filtered['LLCMisses'] / (df_filtered['CacheRefs'] + epsilon)
    
    # Clean up ChunkSize for plotting - use 0 for 'seq'
    df_filtered['ChunkSize'] = df_filtered['ChunkSize'].fillna(0)
    
    df_filtered['Threads'] = df_filtered['Threads'].astype(int)
    df_filtered['ChunkSize'] = df_filtered['ChunkSize'].astype(int)

    print(f"Processed {len(df_filtered)} rows with valid performance data.")

    # Get unique matrices to loop over
    matrices = df_filtered['Matrix'].unique()

    for matrix in matrices:
        print(f"Generating plots for matrix: {matrix}")
        
        matrix_df = df_filtered[df_filtered['Matrix'] == matrix].copy()
        seq_data = matrix_df[matrix_df['Mode'] == 'seq']
        parallel_data = matrix_df[matrix_df['Mode'] != 'seq']

        if parallel_data.empty:
            print(f"  Skipping {matrix}: No parallel data found.")
            continue
            
        plot_df = None
        
        if seq_data.empty:
            print(f"  Warning: No sequential baseline data found for {matrix}.")
            plot_df = parallel_data.copy()
        else:
            # --- Broadcast Sequential Data Logic ---
            seq_baseline_cache = seq_data['CacheMissRatio'].mean()
            seq_baseline_llc = seq_data['LLCMissRatio'].mean()
            
            unique_threads = parallel_data['Threads'].unique()
            unique_chunks = parallel_data['ChunkSize'].unique()
            
            new_seq_rows = []
            for chunk in unique_chunks:
                for thread in unique_threads:
                    new_row = {
                        'Matrix': matrix, 'Mode': 'seq', 'ChunkSize': chunk,
                        'Threads': thread, 'CacheMissRatio': seq_baseline_cache,
                        'LLCMissRatio': seq_baseline_llc,
                    }
                    new_seq_rows.append(new_row)
            
            seq_broadcast_df = pd.DataFrame(new_seq_rows)
            plot_df = pd.concat([parallel_data, seq_broadcast_df], ignore_index=True)
            # --- End Broadcast Logic ---

        if plot_df is None or plot_df.empty:
            print(f"  Skipping {matrix}: No data to plot.")
            continue
            
        thread_ticks = sorted(plot_df['Threads'].unique())
        matrix_filename = matrix.replace('.', '_')

        # Define style mappings
        all_modes = plot_df['Mode'].unique()
        dashes_map = {mode: (2, 2) if mode == 'seq' else '' for mode in all_modes}
        palette_map = {'seq': 'black', 'static': 'blue', 'dynamic': 'green', 'guided': 'orange'}
        
        # ----- Plot 1: Cache Miss Ratio -----
        try:
            plot_cache = sns.relplot(
                data=plot_df,
                x='Threads', y='CacheMissRatio',
                hue='Mode', style='Mode',
                dashes=dashes_map, palette=palette_map,
                row='ChunkSize', # <-- Labels on the right
                col=None,        # <-- Explicitly no columns
                kind='line', marker=True,
                height=3, aspect=2, legend='full'
            )
            
            # --- NEW: Adjust title and layout ---
            plot_cache.fig.subplots_adjust(top=0.92) # Make room for the suptitle
            plot_cache.fig.suptitle(f'Cache Miss Ratio vs. Threads\nMatrix: {matrix}')
            # --- End New Lines ---
            
            plot_cache.set_axis_labels('Number of Threads', 'Cache Miss Ratio')
            
            for ax in plot_cache.axes.flat:
                ax.yaxis.set_major_formatter(PercentFormatter(1.0))
                ax.grid(True, linestyle='--', alpha=0.6)
                ax.set_xscale('log', base=2)
                ax.set_xticks(thread_ticks)
                ax.get_xaxis().set_major_formatter(ScalarFormatter())

            plot1_name = os.path.join(output_dir, f'{output_prefix}_{matrix_filename}_cache_miss.png')
            plot_cache.fig.savefig(plot1_name, dpi=150, bbox_inches='tight')
            plt.close(plot_cache.fig)
            print(f"  Saved plot: {plot1_name}")

        except Exception as e:
            print(f"  !! ERROR plotting Cache Miss Ratio for {matrix}: {e}")

        # ----- Plot 2: LLC Miss Ratio -----
        try:
            plot_llc = sns.relplot(
                data=plot_df,
                x='Threads', y='LLCMissRatio',
                hue='Mode', style='Mode',
                dashes=dashes_map, palette=palette_map,
                row='ChunkSize', # <-- Labels on the right
                col=None,        # <-- Explicitly no columns
                kind='line', marker=True,
                height=3, aspect=2, legend='full'
            )
            
            # --- NEW: Adjust title and layout ---
            plot_llc.fig.subplots_adjust(top=0.92) # Make room for the suptitle
            plot_llc.fig.suptitle(f'LLC Miss Ratio vs. Threads\nMatrix: {matrix}')
            # --- End New Lines ---
            
            plot_llc.set_axis_labels('Number of Threads', 'LLC Miss Ratio')
            
            for ax in plot_llc.axes.flat:
                ax.yaxis.set_major_formatter(PercentFormatter(1.0))
                ax.grid(True, linestyle='--', alpha=0.6)
                ax.set_xscale('log', base=2)
                ax.set_xticks(thread_ticks)
                ax.get_xaxis().set_major_formatter(ScalarFormatter())
                
            plot2_name = os.path.join(output_dir, f'{output_prefix}_{matrix_filename}_llc_miss.png')
            plot_llc.fig.savefig(plot2_name, dpi=150, bbox_inches='tight')
            plt.close(plot_llc.fig)
            print(f"  Saved plot: {plot2_name}")
        
        except Exception as e:
            print(f"  !! ERROR plotting LLC Miss Ratio for {matrix}: {e}")


if __name__ == "__main__":
    # Assume the script is in a 'scripts' or 'src' folder
    # and 'results' and 'plots' are in the parent directory.
    try:
        script_dir = os.path.dirname(os.path.realpath(__file__))
        project_root = os.path.dirname(script_dir)
        
        results_dir = os.path.join(project_root, 'D1/results')
        plots_dir = os.path.join(project_root, 'D1/plots')

        # Define the single, known CSV file name
        csv_name = 'perf_results.csv'
        file_to_plot = os.path.join(results_dir, csv_name)
        
        # Check if the file actually exists before trying to plot
        if not os.path.exists(file_to_plot):
            print(f"Error: The file '{csv_name}' was not found in {results_dir}")
            exit()

        print(f"Processing file: {file_to_plot}")
        plot_perf_data(file_to_plot, plots_dir, output_prefix='perf_analysis')
        
    except NameError:
        # Fallback for interactive environments like notebooks
        # where __file__ might not be defined.
        print("Warning: __file__ not defined. Assuming relative paths '.'")
        results_dir = 'results'
        plots_dir = 'plots'
        file_to_plot = os.path.join(results_dir, 'perf_results.csv')
        if not os.path.exists(file_to_plot):
            print(f"Error: The file 'perf_results.csv' was not found in {results_dir}")
        else:
             print(f"Processing file: {file_to_plot}")
             plot_perf_data(file_to_plot, plots_dir, output_prefix='perf_analysis')
   
    except FileNotFoundError:
        # This will catch if the 'results' directory itself is missing
        print(f"Error: The 'results' directory was not found at {results_dir}")
    except Exception as e:
        print(f"An error occurred: {e}")