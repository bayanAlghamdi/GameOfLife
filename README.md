# GameOfLife

# Conway's Game of Life – Sequential Benchmark

ARTI 503 – Parallel Architecture and Programming | Project Stage 1 | Group 2 (9FA01)

This program runs Conway's Game of Life on large grids and measures its sequential (single-core) execution time. These results are the baseline for the parallel version in later stages.

## Requirements

A C++ compiler (`g++`). To check, open a terminal and type:

```
g++ --version
```

- **Windows:** if the command is not recognized, install it with:
  ```
  winget install BrechtSanders.WinLibs.POSIX.UCRT
  ```
  Then close and reopen VS Code (or the terminal).
- **Mac:** run `xcode-select --install` in the Terminal.

## How to Run

1. Download `GameOfLife.cpp` and open its folder in VS Code.
2. Open a terminal: **Terminal → New Terminal**.
3. Compile (use exactly this command so all results are comparable):
   ```
   g++ -O2 GameOfLife.cpp -o gol
   ```
4. Run:
   - **Windows:** `.\gol.exe`
   - **Mac / Linux:** `./gol`
5. Wait until the program finishes (about 2–3 minutes). Do not stop it.

### Before running

- Keep the laptop plugged in.
- Set the power mode to **Best performance** (Windows: Settings → System → Power).
- Close other heavy apps and pause OneDrive syncing.

## Output

| Output | Meaning |
|---|---|
| Blinker test | Checks that the rules are implemented correctly |
| Table 2 | Execution time for each grid size (3 runs + average) |
| Table 3 | How the time is divided between neighbor counting and rule application |
| Memory Access Experiment | Row-by-row vs. column-by-column traversal |

**For the report:** copy the **Average** column of Table 2 into your row in Table 4.

## Settings

The experiment settings are constants at the top of `GameOfLife.cpp`:

| Setting | Value |
|---|---|
| Grid sizes | 500×500, 1000×1000, 2000×2000, 4000×4000 |
| Generations | 100 |
| Runs per grid size | 3 |
| Initial live cells | 30% |
| Random seed | 42 |

Please do not change them, so that all team members' results can be compared.
