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
      <a href="#project-structure">Project Structure</a>
    </li>
    <li>
      <a href="#reproducibility">Reproducibility</a>
    </li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>

---

<!-- ABOUT THE PROJECT -->
## About The Project

Efficiently parallelizing Sparse Matrix-Vector (SpMV) multiplication is a significant challenge due to workload imbalance and irregular memory access patterns.

We implemented distributed SpMV using **MPI** with **1D cyclic row partitioning**. We benchmarked strong scaling metrics on real matrices from the [SuiteSparse Matrix Collection](https://sparse.tamu.edu/) and weak scaling on synthetic matrices:

- **Strong Scaling**: Fixed problem size, increasing processes (1-128)
- **Weak Scaling**: Proportional problem growth with processes

### Parallelization Strategy

* **Data Format:** Compressed Sparse Row (CSR)
* **1D Cyclic Row Distribution**: Rows are assigned to ranks using `row_index % P` in a cyclic manner
* **Ghost Pattern**: Dynamically mapped at runtime using `MPI_Alltoall` to identify off-process dependencies.
* **MPI Collectives**: Initial setup uses `MPI_Bcast` for global dimensions, followed by point-to-point distribution of row data, and Collectives (`MPI_Alltoallv`) for ghost exchange.

### Performance Metrics

* **T_Comm**: P90 percentile of communication time (ghost exchange)
* **T_Comp**: P90 percentile of computation time (local SpMV)
* **GFLOPS**: Floating-point operations per second (2 × NNZ / T_Comp)
* **Load Balance**: max_NZ / avg_NZ across ranks
* **Communication Volume**: Ghost entries × 8 bytes per rank

### Key Results

* **Performance:**

  * Achieved a maximum speedup of **11.9x** on 64 processes for the `venkat25`
  * Reached a peak computational throughput of **28.85 GFLOPS**
  * Weak scaling demonstrated near-ideal algorithmic scalability, maintaining constant local computation time and a steady ghost overhead of ~0.017 MB/rank

* **Bottleneck:** 

  * **Strong Scaling:** Performance was constrained by **communication latency** for structured matrices (e.g., `atmosmodl`) and **load imbalance** for highly unstructured ones (e.g., `circuit5M`), highlighting the limits of 1D partitioning for irregular sparsity patterns
  * **Weak Scaling:** Parallel efficiency decreased at scale due to **latency dominance**; the small fixed local workload (approx. 2,560 nonzeros per rank) was insufficient to hide the latency cost of establishing communication via `MPI_Alltoallv`

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Project Structure

```text
  └── D2/
      ├── data/                            # Matrices folder
      │   ├── set_up_matrices.sh           # Download/generate the matrices in the data folder
      │   └── generate_weak_matrices.py    # Python script for synthetic matrices
      ├── plots/                           # Generated performance graphs
      ├── results/                         # Benchmark CSV outputs
      ├── scripts/                         # Helper scripts
      │   ├── *_scaling.pbs                # Job submissions
      │   ├── benchmark_*.sh               # Benchmark locally
      │   └── plots_*.py                   # Python scripts for generating plots
      ├── src/                             # Source code
      │   ├── main.c                       # Main code where SpMV is used
      │   ├── mmio.c                       # Matrix Market I/O helper
      │   ├── distribution.c               # Cyclic row distribution across MPI ranks
      │   ├── ghost_entries.c              # Ghost pattern building and communication
      |   └── matrix.c                     # SpMV implementation
      ├── include/ 
      └── README.md
```

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Reproducibility

### Prerequisites and Dependencies

Before running the project, ensure you have the following installed:

* **MPI** (MPICH) support, required for parallel execution;

* **NIST Matrix Market I/O**: The project uses the `mmio.c` and `mmio.h` library (included in `src/` and `include/`) for parsing `.mtx` files;

* **Python** for generating synthetic matrices and plots, with **matplotlib**, **pandas** and **numpy**.

### Instructions

Here are the instructions to reproduce the project locally or in the Unitn cluster:

1. Clone the repo:

  ```sh
      git clone https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git
  ```

2. Inside "PARCO-Computing-2026-244658" run the script to download/generate the matrices needed:

  ```sh
      chmod +x D2/data/set_up_matrices.sh
      D2/data/set_up_matrices.sh
  ```

3. Now you have two options based on where you want to run the program:

    a. If you want to run it locally, you can run the shell scripts, making sure you have executed the permissions:

      ```sh
          chmod +x D2/scripts/*.sh
          D2/scripts/benchmark_strong_scaling.sh
          D2/scripts/benchmark_weak_scaling.sh
      ```

    b. If you are in the cluster you can submit the jobs with:

      ```sh
          qsub D2/scripts/strong_scaling.pbs
          qsub D2/scripts/weak_scaling.pbs
      ```

    c. If you prefer to run specific tests manually without the scripts (always from the "PARCO-Computing-2026-244658" directory):

      * Compile

          ```sh
              mpicc -O3 -Wall -ID2/include D2/src/main.c D2/src/matrix.c D2/src/mmio.c D2/src/ghost_entries.c D2/src/distribution.c -o spmv
          ```

      * Run

          ```sh
              mpirun -np <procs> ./spmv D2/data/<dir>/<matrix>.mtx
              # <procs> = number of processes
              # <dir> = either strong_scaling_matrices or weak_scaling_matrices
              # <matrix> = matrix name 

              # Example:
               mpirun -np 4 ./spmv D2/data/strong_scaling_matrices/venkat25.mtx
          ```
      
      Notice: in this case the results will be printed to standard output, they won't show up in the `/results` folder as the following steps mention.

4. Results will be saved in the `/results` folder in csv format;

5. To plot the results you can use the python scripts in the `/scripts` directory (`plots_*.py`), they will create visual plots in the `/plots` folder.

### Datasets
The project benchmarks the following matrices from the [SuiteSparse Matrix Collection](https://sparse.tamu.edu/):

| Matrix Name | Rows | Columns | Non-zeros | Sparsity [%] | File size [KB] |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **venkat25** | 62,424 | 62,424 | 1,717,763 | 0.04408 | 49,930 |
| **atmosmodl** | 1,489,752 | 1,489,752 | 10,319,760 | 0.00046 | 214,022 |
| **rajat31** | 4,690,002 | 4,690,002 | 20,316,253 | 0.00009 | 619,885 |
| **circuit5M** | 5,558,326 | 5,558,326 | 59,524,291 | 0.00019 | 2,114,015 |
| **cage15** | 5,154,859 | 5,154,859 | 99,199,551 | 0.00037 | 2,566,142 |

**Note:** Ensure the `.mtx` files are placed inside the right `data/` directory.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Agnese Marchesini

Student ID: 244658

Email: agnese.marchesini-1@unitn.it

Project Link: [https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git](https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
