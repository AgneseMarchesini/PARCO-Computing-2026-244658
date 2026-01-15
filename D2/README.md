<a name="readme-top"></a>

<div align="center">

</div>

<br />
<div align="center">
  <a href="https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git">
  </a>

  <h1 align="center">Sparse Matrix-Vector Multiplication Optimization using MPI</h1>

  <p align="center">
    Marchesini Agnese 244658
  </p>
</div>

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
    </li>
    <li>
      <a href="#project-structure">Project Structure</a></li>
    </li>
    <li>
      <a href="#reproducibility">Reproducibility</a></li>
    </li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>

---

<!-- ABOUT THE PROJECT -->
## About The Project

Efficiently parallelizing Sparse Matrix-Vector (SpMV) multiplication is a significant challenge due to workload imbalance and irregular memory access patterns.

In this project we use MPI to benchmark communication and computation time, GFLOPs and communication volume per rank. We analyzed Strong Scaling metrics on real matrices from the [SuiteSparse Matrix Collection](https://sparse.tamu.edu/) and Weak Scaling on syntethic matrices.

### Key Results

* **Performance:** a
* **Bottleneck:** b

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Project Structure

```text
  .
  ├── data/                            # Matrix market files (.mtx) used for testing
  |   ├── strong_scaling_matrices      # 
  │   └── weak_scaling_matrices        # 
  ├── plots/                           # Generated performance graphs
  ├── results/                         # Benchmark CSV outputs
  ├── scripts/                         # Helper scripts
  │   ├── *_scaling.pbs                # Job submissions
  │   ├── *_benchmark.sh               # 
  │   ├── generate_weak_matrices.py    # Python script for synthetic matrices
  │   └── plots_*.py                   # Python scripts for generating plots
  ├── src/                             # Source code
  │   ├── main.c                       # Main code where SpMV is used
  │   ├── mmio.c                       # Matrix Market I/O helper
  │   ├── distribution.c               # 
  │   ├── ghost_emtries.c              # 
  |   └── matrix.c                     # SpMV implementation
  └── README.md
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Reproducibility

### Prerequisites and Dependencies

Before running the project, ensure you have the following installed:

!!!!!!!

* **GCC Compiler** with **OpenMP** support, required for parallel execution;

* **NIST Matrix Market I/O**: The project uses the `mmio.c` and `mmio.h` library (included in `src/`) for parsing `.mtx` files.

* (Optional, only for plotting) **Python**;

### Instructions

Here are the instructions to reproduce the project locally or in the Unitn cluster:

!!!!!!!!!!!!!!!!

1. Clone the repo:

```sh
    git clone https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git
```

2. Download manually from the `data` github directory the five matrices used;

3. Replace the existing pointers in the cloned `data` folder with the actual matrix files;

4. Now you have two options based on where you want to run the program:

    a. If you want to run it locally you can run the shell scripts, making sure you have executed the permissions:

      ```sh
          chmod +x /scripts/*.sh
          /scripts/time_benchmark.sh 
          /scripts/perf_benchmark.sh
      ```

    b. If you are in the cluster you can submit the jobs with:

      ```sh
          qsub /scripts/pbs_script_perf.pbs
          qsub /scripts/pbs_scripts_time.pbs
      ```

    c. If you prefer to run specific tests manually without the scripts:

      * Compile

          ```sh
              gcc -Wall -O3 -fopenmp ./src/main.c ./src/mmio.c ./src/matrix.c -o spmv -lm
          ```

      * Run

          ```sh
              ./spmv <matrix_file.mtx> <scheduling mode> <chunk size> <threads> <number_of_runs>
              # Scheduling modes: seq (runs the sequential version), static, dynamic, guided

              # Example:
              ./spmv data/af23560.mtx static 100 16 10
          ```
      
      Notice: in this case the results will be printed to standard output, they won't show up in the `/results` folder as the following steps mention.

5. Results will be saved in the `/results` folder in csv format;

6. To plot the results you can use the python scripts in the `/scripts` directory, they will create visual plots in the `/plots` folder.

### Datasets
The project benchmarks the following matrices from the [SuiteSparse Matrix Collection](https://sparse.tamu.edu/):

| Matrix Name | Rows | Columns | Non-zeros | Sparsity [%] | File size [KB] |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **af23560** | 23,560 | 23,560 | 460,598 | 0.08298 | 15,164 |
| **twotone** | 120,750 | 120,750 | 1,206,265 | 0.00827 | 24,107 |
| **tmt_unsym** | 917,825 | 917,825 | 4,584,801 | 0.00054 | 145,259 |
| **atmosmodl** | 1,489,752 | 1,489,752 | 10,319,760 | 0.00046 | 214,022 |
| **Freescale1** | 3,428,755 | 3,428,755 | 17,052,626 | 0.00015 | 632,885 |

**Note:** Ensure the downloaded `.mtx` files are placed inside the `data/` directory.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Agnese Marchesini

Student ID: 244658

Email: agnese.marchesini-1@unitn.it

Project Link: [https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git](https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
