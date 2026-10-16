// #include <iostream>

// int main() {
//     std::cout << "Game of Life" << std::endl;
//     return 0;
// }

// Conway's Game of Life - Sequential Version (Project Stage 1)
// ARTI 503 - Parallel Architecture and Programming - Group 2
//
// Compile: g++ -O2 GameOfLife.cpp -o gol
// Run:     .\gol.exe   (Windows)   or   ./gol   (Mac/Linux)

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

using Clock = std::chrono::steady_clock;

// ---------------- Settings ----------------
const int GRID_SIZES[] = {500, 1000, 2000, 4000};
const int NUM_SIZES = 4;
const int NUM_GENERATIONS = 100;
const int NUM_RUNS = 3;
const double DENSITY = 0.3;   // 30% of cells start alive
const int SEED = 42;          // same seed = same starting grid every run

// ---------------- Grid ----------------
// The grid is stored as one long row (row-major order),
// with an extra dead border around it so every real cell has 8 neighbors.
// Real cells are at rows 1..n and columns 1..n.
struct Grid {
    int n;                       // number of real rows and columns
    int stride;                  // row length including the border (n + 2)
    std::vector<uint8_t> cells;  // 1 = alive, 0 = dead

    // Constructor: creates an empty (all dead) grid
    Grid(int size) {
        n = size;
        stride = size + 2;
        cells = std::vector<uint8_t>(stride * stride, 0);
    }

    // Access the cell at (row, col)
    uint8_t& at(int row, int col) {
        return cells[row * stride + col];
    }
};

// Fill the grid randomly using a fixed seed
void fillRandom(Grid& g) {
    std::mt19937 rng(SEED);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 1; i <= g.n; i++) {
        for (int j = 1; j <= g.n; j++) {
            if (dist(rng) < DENSITY)
                g.at(i, j) = 1;
            else
                g.at(i, j) = 0;
        }
    }
}

// Count the live neighbors of the cell at (row, col)
int countNeighbors(Grid& g, int row, int col) {
    int count = 0;
    count += g.at(row - 1, col - 1);  // top-left
    count += g.at(row - 1, col);      // top
    count += g.at(row - 1, col + 1);  // top-right
    count += g.at(row, col - 1);      // left
    count += g.at(row, col + 1);      // right
    count += g.at(row + 1, col - 1);  // bottom-left
    count += g.at(row + 1, col);      // bottom
    count += g.at(row + 1, col + 1);  // bottom-right
    return count;
}

// Apply the Game of Life rules to one cell
uint8_t applyRules(uint8_t alive, int neighbors) {
    if (alive == 1) {
        if (neighbors == 2 || neighbors == 3)
            return 1;   // survives
        else
            return 0;   // dies (loneliness or overcrowding)
    } else {
        if (neighbors == 3)
            return 1;   // birth
        else
            return 0;   // stays dead
    }
}

// Compute one generation, row by row (cache-friendly).
// Reads from "current" and writes to "next" so the old values are not overwritten.
void nextGeneration(Grid& current, Grid& next) {
    for (int i = 1; i <= current.n; i++) {
        for (int j = 1; j <= current.n; j++) {
            int neighbors = countNeighbors(current, i, j);
            next.at(i, j) = applyRules(current.at(i, j), neighbors);
        }
    }
}

// Same as nextGeneration, but column by column (cache-unfriendly).
// Used only for the memory access experiment.
void nextGenerationByColumn(Grid& current, Grid& next) {
    for (int j = 1; j <= current.n; j++) {
        for (int i = 1; i <= current.n; i++) {
            int neighbors = countNeighbors(current, i, j);
            next.at(i, j) = applyRules(current.at(i, j), neighbors);
        }
    }
}

// Count all live cells (used to check that every run gives the same result)
long long countAlive(Grid& g) {
    long long total = 0;
    for (int i = 1; i <= g.n; i++)
        for (int j = 1; j <= g.n; j++)
            total += g.at(i, j);
    return total;
}

// Run the full simulation and return the time in seconds
double runSimulation(int size, bool byColumn, long long& aliveAtEnd) {
    Grid current(size);
    Grid next(size);
    fillRandom(current);

    auto start = Clock::now();
    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
        if (byColumn)
            nextGenerationByColumn(current, next);
        else
            nextGeneration(current, next);
        std::swap(current, next);   // the new generation becomes the current one
    }
    auto end = Clock::now();

    aliveAtEnd = countAlive(current);
    return std::chrono::duration<double>(end - start).count();
}

// Measure how the time is divided inside one generation:
// first count the neighbors of all cells, then apply the rules to all cells.
void measureSections(int size, double& neighborTime, double& rulesTime) {
    Grid current(size);
    Grid next(size);
    fillRandom(current);
    std::vector<uint8_t> counts(current.cells.size(), 0);

    neighborTime = 0;
    rulesTime = 0;
    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
        auto t1 = Clock::now();
        for (int i = 1; i <= size; i++)
            for (int j = 1; j <= size; j++)
                counts[i * current.stride + j] = countNeighbors(current, i, j);

        auto t2 = Clock::now();
        for (int i = 1; i <= size; i++)
            for (int j = 1; j <= size; j++)
                next.at(i, j) = applyRules(current.at(i, j), counts[i * current.stride + j]);

        auto t3 = Clock::now();
        std::swap(current, next);
        neighborTime += std::chrono::duration<double>(t2 - t1).count();
        rulesTime += std::chrono::duration<double>(t3 - t2).count();
    }
}

// Correctness test: a "blinker" (3 cells in a row) must turn vertical
bool testBlinker() {
    Grid a(5);
    Grid b(5);
    a.at(3, 2) = 1;
    a.at(3, 3) = 1;
    a.at(3, 4) = 1;
    nextGeneration(a, b);
    return b.at(2, 3) == 1 && b.at(3, 3) == 1 && b.at(4, 3) == 1 && countAlive(b) == 3;
}

int main() {
    std::cout << std::fixed << std::setprecision(4);   // print 4 decimal places
    std::cout << "=== Conway's Game of Life - Sequential Benchmark ===\n\n";

    // 1. Correctness test
    if (testBlinker()) {
        std::cout << "Blinker test: PASS\n\n";
    } else {
        std::cout << "Blinker test: FAIL\n";
        return 1;
    }

    std::cout << "Generations: " << NUM_GENERATIONS << ", Runs: " << NUM_RUNS
              << ", Density: " << DENSITY << ", Seed: " << SEED << "\n\n";

    // 2. Execution time per grid size
    std::cout << "--- Table 2: Execution Time per Grid Size (seconds) ---\n";
    std::cout << std::left << std::setw(12) << "Grid Size" << "\tRun 1\tRun 2\tRun 3\tAverage\n";
    double averages[NUM_SIZES];
    for (int s = 0; s < NUM_SIZES; s++) {
        int size = GRID_SIZES[s];
        double sum = 0;
        long long alive = 0;
        std::cout << std::left << std::setw(12) << (std::to_string(size) + "x" + std::to_string(size));
        for (int r = 0; r < NUM_RUNS; r++) {
            double t = runSimulation(size, false, alive);
            sum += t;
            std::cout << "\t" << t << std::flush;
        }
        averages[s] = sum / NUM_RUNS;
        std::cout << "\t" << averages[s] << "\n";
    }

    // 3. Time-consuming sections (largest grid)
    int bigSize = GRID_SIZES[NUM_SIZES - 1];
    double neighborSum = 0, rulesSum = 0;
    for (int r = 0; r < NUM_RUNS; r++) {
        double nt, rt;
        measureSections(bigSize, nt, rt);
        neighborSum += nt;
        rulesSum += rt;
    }
    double neighborAvg = neighborSum / NUM_RUNS;
    double rulesAvg = rulesSum / NUM_RUNS;
    double total = neighborAvg + rulesAvg;

    std::cout << "\n--- Table 3: Time-Consuming Sections (" << bigSize << "x" << bigSize << ") ---\n";
    std::cout << "Generation loop (all generations): " << averages[NUM_SIZES - 1] << " s\n";
    std::cout << "Grid update (one generation):      "
              << averages[NUM_SIZES - 1] / NUM_GENERATIONS * 1000 << " ms\n";
    std::cout << "Neighbor counting: " << neighborAvg << " s (" << neighborAvg / total * 100 << "%)\n";
    std::cout << "Rule application:  " << rulesAvg << " s (" << rulesAvg / total * 100 << "%)\n";

    // 4. Memory access experiment (largest grid)
    double rowSum = 0, colSum = 0;
    long long alive = 0;
    for (int r = 0; r < NUM_RUNS; r++) {
        rowSum += runSimulation(bigSize, false, alive);
        colSum += runSimulation(bigSize, true, alive);
    }
    std::cout << "\n--- Memory Access Experiment (" << bigSize << "x" << bigSize << ") ---\n";
    std::cout << "Row by row:       " << rowSum / NUM_RUNS << " s\n";
    std::cout << "Column by column: " << colSum / NUM_RUNS << " s\n";
    std::cout << "Column order is " << colSum / rowSum << "x slower\n\n";

    
    return 0;
}
