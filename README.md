# Conway's Game of Life – Sequential and Parallel Benchmarks

ARTI 503 – Parallel Architecture and Programming | Group 2 (9FA01)

This repository contains a C++ implementation of Conway's Game of Life used to study parallel performance on multi-core CPUs.

| File | Stage | Description |
|---|---|---|
| `GameOfLife.cpp` | Project Stage 1 | Sequential (single-core) version, used as the performance baseline |
| `GameOfLife_Parallel.cpp` | Assignment 2 | OpenMP version with race condition and synchronization experiments |

## Requirements

A C++ compiler (`g++`) with OpenMP support. To check, open a terminal and type:

```
g++ --version
```

- **Windows:** if the command is not recognized, install it with:
  ```
  winget install BrechtSanders.WinLibs.POSIX.UCRT
  ```
  Then close and reopen VS Code (or the terminal).
- **Mac:** the default Apple compiler does not support OpenMP. Install GCC with Homebrew (`brew install gcc`) and use `g++-14` (or the version installed) instead of `g++`.

### Before running

- Keep the laptop plugged in.
- Set the power mode to **Best performance** (Windows: Settings → System → Power).
- Close other heavy apps and pause OneDrive syncing.
- Let the program finish without stopping it.

---

## Stage 1: Sequential Version

### How to Run

Compile (use exactly this command so all results are comparable):

```
g++ -O2 GameOfLife.cpp -o gol
```

Run:
- **Windows:** `.\gol.exe`
- **Mac / Linux:** `./gol`

The program takes about 2–3 minutes.

### Output

| Output | Meaning |
|---|---|
| Blinker test | Checks that the rules are implemented correctly |
| Execution time per grid size | Time for each grid size (3 runs + average) |
| Time-consuming sections | How the time is divided between neighbor counting and rule application |
| Memory access experiment | Row-by-row vs. column-by-column traversal |

---

## Assignment 2: Parallel Version

### How to Run

Compile (note the `-fopenmp` flag):

```
g++ -O2 -fopenmp GameOfLife_Parallel.cpp -o gol_parallel
```

Run:
- **Windows:** `.\gol_parallel.exe`
- **Mac / Linux:** `./gol_parallel`

The program takes about 1–2 minutes.

### Experiments

**1. Shared live-cell counter**
All threads add to the same counter. The program compares:

| Version | Description |
|---|---|
| Race | No protection (race condition, incorrect results) |
| Critical | `#pragma omp critical`: one thread at a time |
| Atomic | `#pragma omp atomic`: indivisible hardware addition |
| Reduction | `reduction(+:total)`: private copy per thread, combined at the end |

Tested on 1, 2, 4, and 8 threads.

**2. Grid update**
The program compares two ways of computing each generation in parallel:

| Version | Description |
|---|---|
| Single grid | One grid updated in place (race condition, incorrect results) |
| Two grids | Read from one grid, write to another (double buffering, correct results) |

Tested on 1, 2, 4, 8, 16, and 24 threads.

### Output

For each version and number of threads, the program prints the result of 3 runs, whether all results are correct, the average execution time, and the speed-up compared with the sequential version.

---

## Settings

The experiment settings are constants at the top of each source file:

| Setting | Value |
|---|---|
| Grid sizes | 500×500 to 4000×4000 (Stage 1), 4000×4000 (Assignment 2) |
| Generations | 100 |
| Runs per experiment | 3 |
| Initial live cells | 30% |
| Random seed | 42 (same starting grid in every run) |

Please do not change them, so that all results can be compared.
