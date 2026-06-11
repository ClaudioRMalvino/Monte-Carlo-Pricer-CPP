# C++ Monte Carlo Option Pricer

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![pybind11](https://img.shields.io/badge/pybind11-Enabled-green.svg)]()
[![OpenMP](https://img.shields.io/badge/OpenMP-Parallel-orange.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](./LICENSE)

This project is a high-performance C++ library for pricing European Call and Put options using the Monte Carlo method.

The core computational engine is written in modern C++ (C++20) for speed, parallelized with OpenMP, and exposed as a Python module using `pybind11`. This creates a high-performance backend that can be easily imported and used in standard Python-based financial analysis and research scripts. It includes benchmarking against a pure Python implementation (a reproducible ~3× single-thread speedup) and OpenMP scaling benchmarks (up to ~4× on an 8-thread CPU), with deterministic random number generation and type-hinted interfaces suitable for research, quantitative finance, and HPC development.

## Key Features

* **Fast C++ Engine:** The core simulation loop is written in C++ for native performance.
* **Parallelized with OpenMP:** Each thread draws from its own RNG stream (seeded per thread) and payoffs are accumulated with an OpenMP reduction — thread-safe and reproducible for a fixed thread count.
* **Python API:** A clean, importable Python module created with `pybind11` (the GIL is released during pricing so threads run concurrently).
* **Robust Design:** The C++ `EuropeanOption` class is `const`-correct and immutable.
* **Testable & Reproducible:** Implements constructor overloading to allow for both random and fixed-seed generation, which is critical for testing and reproducibility.
* **Type-Hinted:** A complete `.pyi` stub file is provided for full autocompletion and static analysis (e.g., `pyright`, `mypy`) in your editor.

## Benchmark Results

Detailed benchmark methodology, plots, and data are available here:
[Benchmarks →](./benchmark/README.md)

## Performance Overview

Below is a sample benchmark demonstrating runtime scaling and speedup:

![Scaling Plot](benchmark/examples/python_vs_cpp_num_paths.svg)

![Speedup Plot](benchmark/examples/python_vs_cpp_num_paths_speedup.svg)

### OpenMP Parallel Scaling

Strong scaling of the C++ engine (10M paths) across thread counts on a 4-core / 8-thread CPU. Speedup is near-linear up to the 4 physical cores, with hyper-threading taking it to ~4×:

![OpenMP Runtime](benchmark/examples/omp_parallel_runtime.svg)

![OpenMP Speedup](benchmark/examples/omp_parallel_speedup.svg)

## Technology Stack

* **Core Engine:** C++20
* **Parallelism:** OpenMP
* **Python Bridge:** `pybind11`
* **Interface:** Python 3
* **Build System:** `CMake`

## Prerequisites

To build this library, you will need:

* A C++20 compatible compiler with OpenMP support (e.g., `g++`, `clang++`)
* `CMake`
* `Python`
* `pybind11`
* `numpy` and `matplotlib` (for running the benchmarks)

The easiest way to get the build dependencies is via `conda`:

```bash
conda install -c conda-forge pybind11 cmake cxx-compiler
```

To run the OpenMP scaling benchmark, set the thread count via `OMP_NUM_THREADS` (the included `parallel_scaling.py` sweeps this automatically):

```bash
cd benchmark
PYTHONPATH=../build/python:../python python parallel_scaling.py
```

## How to Build

1. Clone this repository:

    ```bash
    git clone [https://github.com/ClaudioRMalvino/Monte-Carlo-Pricer.git(https://github.com/ClaudioRMalvino/Monte-Carlo-Pricer.git)
    cd Monte-Carlo-Pricer
    ```

2. Create a build directory (this project uses the `CMakeLists.txt` from the parent directory):

    ```bash
    mkdir build
    cd build
    ```

3. Configure the build with `cmake`:

    ```bash
    cmake ..
    ```

4. Compile the Python module:

    ```bash
    make
    ```

This will create the `monte_carlo_pricer.cpython-....so` file in the project's build directory, `./build/python`.

5. From repository root you need to run the following command to link the modules

    ```bash
    export PYTHONPATH="$(pwd)/python:$(pwd)/build/python"
    ```

## Usage Example

The compiled library can be imported directly into any Python script or notebook.

```python
# main.py
import monte_carlo_pricer
import math

# 1. Set up the option parameters
S0 = 100.0     # Initial Price
K = 100.0      # Strike Price
r = 0.05       # Risk-free rate (5%)
sigma = 0.20   # Volatility (20%)
T = 1.0        # 1 year to expiration
seed = 42      # Use a fixed seed for reproducible results

# 2. Create an instance of the C++ EuropeanOption class
#    (pyright will provide full autocompletion)
option = monte_carlo_pricer.EuropeanOption(
    initStockPrice=S0,
    strikePrice=K,
    riskFreeIntRate=r,
    volatility=sigma,
    timeToExpire=T,
    seed=seed
)

# 3. Run the simulation
#    This calls the C++ backend and returns a Python tuple
num_sims = 5_000_000
call_price, put_price = option.calculatePrice(num_sims)

# 4. Print the results
print(f"--- Monte Carlo Simulation ---")
print(f"Stock Price:    {S0}")
print(f"Strike Price:   {K}")
print(f"Simulations:    {num_sims:,}")
print("--------------------------------")
print(f"Call Price: {call_price:.3f}")
print(f"Put Price:  {put_price:.3f}")

```

### Example Output

```
--- Monte Carlo Simulation ---
Stock Price:    100.0
Strike Price:   100.0
Simulations:    5,000,000
--------------------------------
Call Price: 10.451
Put Price:  5.574
```
