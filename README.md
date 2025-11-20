<a name="readme-top"></a>

<div align="center">

</div>

<br />
<div align="center">
  <a href="https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git">
  </a>

  <h3 align="center">Sparse Matrix-Vector Multiplication Optimization</h3>

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
      <a href="#installation">Installation</a></li>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#contact">Contact</a></li>
  </ol>
</details>

---

<!-- ABOUT THE PROJECT -->
## About The Project

Efficiently parallelizing Sparse Matrix-Vector (SpMV) multiplication is a significant challenge due to workload imbalance and irregular memory access patterns.
In this project we use OpenMP to benchmark different combinations of threads, scheduling types and chunk sizes to analyze the speedups.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

## Installation

Here are the instruction to reproduce the project locally or in the Unitn cluster:

1. Clone the repo:

```sh
    git clone https://github.com/AgneseMarchesini/PARCO-Computing-2026-244658.git
```

2. Download from the `data` directory the five matrices used;

3. Replace the existing pointers in the cloned `data` folder with the actual matrix files;

4. Now you can proceed as follows:

  a. If you want to run it locally you can run the `/scripts/time_benchmark.sh` and `/scripts/perf_benchmark.sh`;

  b. If you are in the cluster you can subtim the jobs with: `qsub /scripts/pbs_script_perf.pbs` and `qsub /scripts/pbs_scripts_time.pbs`;

5. Results will be saved in the `/results` folder in csv format;

6. To plot the results you can use the python scripts in the `/scripts` directory, they will create visual plots in the `/plots` folder.


<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->
## Usage

Use this space to show useful examples of how a project can be used. Additional screenshots, code examples and demos work well in this space. You may also link to more resources.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Your Name - [@twitter_handle](https://twitter.com/twitter_handle) - email@example.com

Project Link: [https://github.com/github_username/repo_name](https://github.com/github_username/repo_name)

<p align="right">(<a href="#readme-top">back to top</a>)</p>
