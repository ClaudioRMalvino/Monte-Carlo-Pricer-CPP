"""
Benchmarks the OpenMP parallel scaling of the C++ Monte Carlo pricer.

For each thread count the pricer is run in a fresh subprocess (OpenMP reads
OMP_NUM_THREADS at process start, so it cannot be changed reliably in-process).
The driver collects the median runtime per thread count and reports speedup
relative to a single thread and parallel efficiency, then plots both.

Run from the benchmark/ directory with the built module on PYTHONPATH:

    PYTHONPATH=../build/python:../python python parallel_scaling.py
"""

import json
import os
import statistics
import subprocess
import sys
import time

import matplotlib

matplotlib.use("Agg")  # headless: write SVGs without a display
import matplotlib.pyplot as plt
import numpy as np

import monte_carlo_pricer

# Benchmark configuration
S0, K, r, sigma, T = 100.0, 100.0, 0.05, 0.2, 1.0
SEED = 42
NUM_PATHS = 10_000_000
REPEATS = 7
COOLDOWN_S = 3.0  # let the CPU cool between configs to limit thermal throttling
THREAD_COUNTS = [1, 2, 3, 4, 5, 6, 7, 8]


def time_single_config() -> dict:
    """Time NUM_PATHS pricing calls under the current OMP_NUM_THREADS.

    Reports the best (minimum) runtime over REPEATS, which is the least
    contended / least throttled run and the most reproducible statistic.

    :return: dict with best runtime (seconds) and the resulting prices.
    """
    opt = monte_carlo_pricer.EuropeanOption(S0, K, r, sigma, T, SEED)
    opt.calculatePrice(1_000_000)  # warm up caches / thread pool

    samples = []
    call = put = 0.0
    for _ in range(REPEATS):
        start = time.perf_counter()
        call, put = opt.calculatePrice(NUM_PATHS)
        samples.append(time.perf_counter() - start)

    return {"time": min(samples), "call": call, "put": put}


def plot_runtime(data) -> None:
    """Plot median runtime versus thread count.

    :param data: numpy array, column 0 threads, column 1 median time [s]
    """
    threads = data[:, 0]
    times = data[:, 1]

    plt.figure()
    plt.plot(threads, times, marker="o", label="C++ (OpenMP)")
    plt.title(f"OpenMP Scaling: Runtime vs Threads ({NUM_PATHS / 1e6:.0f}M paths)")
    plt.xlabel("Threads")
    plt.ylabel("Time, [s]")
    plt.xticks(threads)
    plt.grid()
    plt.legend(loc="upper right")
    plt.savefig("./plots/omp_parallel_runtime.svg")
    plt.savefig("./examples/omp_parallel_runtime.svg")
    plt.close()


def plot_speedup(data) -> None:
    """Plot measured speedup and parallel efficiency versus thread count.

    :param data: numpy array, column 0 threads, column 1 median time [s]
    """
    threads = data[:, 0]
    times = data[:, 1]
    speedup = times[0] / times
    efficiency = speedup / threads

    fig, ax1 = plt.subplots()
    l_speedup, = ax1.plot(threads, speedup, marker="o", color="tab:blue",
                          label="Measured speedup")
    l_ideal, = ax1.plot(threads, threads, linestyle="--", color="gray",
                        label="Ideal (linear)")
    ax1.set_xlabel("Threads")
    ax1.set_ylabel("Speedup", color="tab:blue")
    ax1.set_xticks(threads)
    ax1.grid(alpha=0.4)

    ax2 = ax1.twinx()
    l_eff, = ax2.plot(threads, efficiency * 100.0, marker="s", color="tab:red",
                      label="Efficiency")
    ax2.set_ylabel("Parallel efficiency, [%]", color="tab:red")
    ax2.set_ylim(0, 110)

    # Single horizontal legend row centred above the axes (no frame).
    handles = [l_speedup, l_ideal, l_eff]
    fig.legend(handles, [h.get_label() for h in handles], loc="lower center",
               bbox_to_anchor=(0.5, 0.92), ncol=3, frameon=False)
    fig.savefig("./plots/omp_parallel_speedup.svg", bbox_inches="tight")
    fig.savefig("./examples/omp_parallel_speedup.svg", bbox_inches="tight")
    plt.close(fig)


def benchmark() -> None:
    """Drive the scaling sweep across THREAD_COUNTS and plot the results."""
    os.makedirs("./data", exist_ok=True)
    os.makedirs("./plots", exist_ok=True)
    os.makedirs("./examples", exist_ok=True)

    out_path = "./data/parallel_scaling.dat"
    open(out_path, "w").close()  # truncate previous run

    rows = []
    for idx, n_threads in enumerate(THREAD_COUNTS):
        if idx > 0:
            time.sleep(COOLDOWN_S)  # thermal cooldown between configurations
        env = dict(os.environ, OMP_NUM_THREADS=str(n_threads))
        proc = subprocess.run(
            [sys.executable, __file__, "--worker"],
            env=env, capture_output=True, text=True,
        )
        if proc.returncode != 0:
            sys.stderr.write(proc.stderr)
            raise RuntimeError(f"worker failed for {n_threads} threads")

        res = json.loads(proc.stdout.strip().splitlines()[-1])
        speedup = rows[0][1] / res["time"] if rows else 1.0
        rows.append((n_threads, res["time"]))

        print(f"threads={n_threads:>2}  time={res['time']:.4f}s  "
              f"speedup={speedup:.2f}x  efficiency={speedup / n_threads * 100:.0f}%  "
              f"call={res['call']:.4f}  put={res['put']:.4f}")
        with open(out_path, "a") as f:
            f.write(f"{n_threads} {res['time']} {res['call']} {res['put']}\n")

    data = np.loadtxt(out_path, delimiter=" ")
    plot_runtime(data)
    plot_speedup(data)

    times = data[:, 1]
    speedups = times[0] / times
    best = int(np.argmax(speedups))
    print(f"\nPeak speedup: {speedups[best]:.2f}x at {int(data[best, 0])} threads")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--worker":
        print(json.dumps(time_single_config()))
    else:
        benchmark()
