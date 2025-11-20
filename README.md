<a name="readme-top"></a>

<div align="center">

</div>

<br />
<div align="center">
  <a href="https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git">
  </a>

  <h1 align="center">Sparse Matrix-Vector Multiplication Optimization</h1>

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
      <a href="#structure">Project Structure</a></li>
    </li>
    <li>
      <a href="#installation">Reproducibility</a></li>
    </li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>

---

<!-- ABOUT THE PROJECT -->
## About The Project

Efficiently parallelizing Sparse Matrix-Vector (SpMV) multiplication is a significant challenge due to workload imbalance and irregular memory access patterns.

In this project we use OpenMP to benchmark different combinations of threads, scheduling types and chunk sizes to analyze the speedups.

This was done on five matrices with different dimensions and degrees of sparsity.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Project Structure
.
├── data/                   # Matrix market files (.mtx) used for testing

├── plots/                  # Generated performance graphs

├── results/                # Benchmark CSV outputs

├── scripts/                # Helper scripts

│   ├── pbs_script_*.pbs    # Job submissions

│   ├── *_benchmark.sh      # Local execution script

│   └── create_*.py         # Python script for visualization

├── src/                    # Source code

│   ├── main.c              # Main code where SpMV is used

│   ├── mmio.c              # Matrix Market I/O helper

|   └── matrix.c            # SpMV implementation

└── README.md

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Reproducibility

Before running the project, ensure you have the following installed:

* GCC Compiler with OpenMP support;

* (Optional, only for plotting) Python.

Here are the instruction to reproduce the project locally or in the Unitn cluster:

1. Clone the repo:

```sh
    git clone https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git
```

2. Download from the `data` directory the five matrices used;

3. Replace the existing pointers in the cloned `data` folder with the actual matrix files;

4. Now you can proceed as follows:

    a. If you want to run it locally you can run the shell scripts `/scripts/time_benchmark.sh` and `/scripts/perf_benchmark.sh`, making sure you have executed the permissions (`chmod +x /scripts/*.sh`);

    b. If you are in the cluster you can subtim the jobs with: `qsub /scripts/pbs_script_perf.pbs` and `qsub /scripts/pbs_scripts_time.pbs`;

5. Results will be saved in the `/results` folder in csv format;

6. To plot the results you can use the python scripts in the `/scripts` directory, they will create visual plots in the `/plots` folder.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Agnese Marchesini

Student ID: 244658

Email: agnese.marchesini-1@unitn.it

Project Link: [https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git](https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
