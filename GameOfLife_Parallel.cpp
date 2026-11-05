// Conway's Game of Life - Parallel Version (Assignment 2)
// ARTI 503 - Parallel Architecture and Programming - Group 2

// Compile: g++ -O2 -fopenmp GameOfLife_Parallel.cpp -o gol_parallel
// Run:     .\gol_parallel.exe   (Windows)   or   ./gol_parallel   (Mac/Linux)

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include <omp.h>

using Clock = std::chrono::steady_clock;

// ---------------- Settings ----------------
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

// Same as nextGeneration, but column by column (from Stage 1, not used in this assignment).
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

// RACE CONDITION: all threads add to the same shared variable "total"
// without any protection, so some additions are lost.
long long countAliveRace(Grid& g) {
    long long total = 0;
    #pragma omp parallel for
    for (int i = 1; i <= g.n; i++)
        for (int j = 1; j <= g.n; j++)
            total += g.at(i, j);
    return total;
}

// SOLUTION 1 - critical: only one thread at a time can run the protected line
long long countAliveCritical(Grid& g) {
    long long total = 0;
    #pragma omp parallel for
    for (int i = 1; i <= g.n; i++)
        for (int j = 1; j <= g.n; j++) {
            #pragma omp critical
            total += g.at(i, j);
        }
    return total;
}

// SOLUTION 2 - atomic: the addition is done as one indivisible hardware operation
long long countAliveAtomic(Grid& g) {
    long long total = 0;
    #pragma omp parallel for
    for (int i = 1; i <= g.n; i++)
        for (int j = 1; j <= g.n; j++) {
            #pragma omp atomic
            total += g.at(i, j);
        }
    return total;
}

// SOLUTION 3 - reduction: each thread counts in its own private copy,
// and OpenMP adds the copies together at the end
long long countAliveReduction(Grid& g) {
    long long total = 0;
    #pragma omp parallel for reduction(+:total)
    for (int i = 1; i <= g.n; i++)
        for (int j = 1; j <= g.n; j++)
            total += g.at(i, j);
    return total;
}

// Run one version 3 times with a given number of threads,
// then print its results as one table row
void testVersion(const char* name, long long (*countFunc)(Grid&), Grid& g, long long correct, double seqTime, int threads) {
    omp_set_num_threads(threads);
    double totalTime = 0;
    bool allCorrect = true;
    std::cout << std::left << std::setw(12) << name << std::setw(9) << threads;
    for (int r = 0; r < NUM_RUNS; r++) {
        auto start = Clock::now();
        long long result = countFunc(g);
        auto end = Clock::now();
        totalTime += std::chrono::duration<double>(end - start).count();
        if (result != correct)
            allCorrect = false;
        std::cout << std::setw(10) << result;
    }
    double avg = totalTime / NUM_RUNS;
    std::cout << std::setw(10) << (allCorrect ? "Yes" : "No")
            << std::setw(14) << avg
            << std::setprecision(4) << seqTime / avg << "x\n"
            << std::setprecision(6);
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

// ---------------- Parallel generation update ----------------

// CORRECT parallel version: two grids (double buffering).
// Threads read only from "current" and write only to "next",
// so no thread can change a value that another thread is reading.
void nextGenerationParallel(Grid& current, Grid& next) {
    #pragma omp parallel for
    for (int i = 1; i <= current.n; i++)
        for (int j = 1; j <= current.n; j++) {
            int neighbors = countNeighbors(current, i, j);
            next.at(i, j) = applyRules(current.at(i, j), neighbors);
        }
}

// RACE CONDITION 2: only ONE grid, updated in place.
// A thread may read a neighbor that another thread has already
// changed to the next generation, so the result depends on timing.
void nextGenerationInPlaceRace(Grid& grid) {
    #pragma omp parallel for
    for (int i = 1; i <= grid.n; i++)
        for (int j = 1; j <= grid.n; j++) {
            int neighbors = countNeighbors(grid, i, j);
            grid.at(i, j) = applyRules(grid.at(i, j), neighbors);
        }
}

// Run the full parallel simulation and return the time in seconds
double runParallelSimulation(int size, bool inPlace, long long& aliveAtEnd) {
    Grid current(size);
    Grid next(size);
    fillRandom(current);

    auto start = Clock::now();
    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
        if (inPlace) {
            nextGenerationInPlaceRace(current);
        } else {
            nextGenerationParallel(current, next);
            std::swap(current, next);
        }
    }
    auto end = Clock::now();

    aliveAtEnd = countAlive(current);
    return std::chrono::duration<double>(end - start).count();
}

// Run one simulation version 3 times and print one table row
void testSimulation(const char* name, bool inPlace, int size,
                    long long correct, double seqTime, int threads) {
    omp_set_num_threads(threads);
    double totalTime = 0;
    bool allCorrect = true;
    std::cout << std::left << std::setw(14) << name << std::setw(9) << threads;
    for (int r = 0; r < NUM_RUNS; r++) {
        long long alive = 0;
        totalTime += runParallelSimulation(size, inPlace, alive);
        if (alive != correct)
            allCorrect = false;
        std::cout << std::setw(10) << alive << std::flush;
    }
    double avg = totalTime / NUM_RUNS;
    std::cout << std::setw(10) << (allCorrect ? "Yes" : "No")
            << std::setw(14) << avg
            << std::setprecision(4) << seqTime / avg << "x\n"
            << std::setprecision(6);
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
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=== Conway's Game of Life - Parallel Experiments (Assignment 2) ===\n\n";

    // Correctness test of the game rules
    if (testBlinker()) {
        std::cout << "Blinker test: PASS\n";
    } else {
        std::cout << "Blinker test: FAIL\n";
        return 1;
    }
    std::cout << "Max available threads: " << omp_get_max_threads() << "\n";
    std::cout << "Runs per version: " << NUM_RUNS << ", Density: " << DENSITY
            << ", Seed: " << SEED << "\n\n";

    // ---------------- Experiment 1: shared live-cell counter ----------------
    const int SIZE = 4000;
    Grid test(SIZE);
    fillRandom(test);

    double seqTotal = 0;
    long long correct = 0;
    for (int r = 0; r < NUM_RUNS; r++) {
        auto start = Clock::now();
        correct = countAlive(test);
        auto end = Clock::now();
        seqTotal += std::chrono::duration<double>(end - start).count();
    }
    double seqTime = seqTotal / NUM_RUNS;

    std::cout << "=== Experiment 1: Shared Live-Cell Counter ===\n";
    std::cout << "Grid: " << SIZE << "x" << SIZE << "\n";
    std::cout << "Correct count:   " << correct << "\n";
    std::cout << "Sequential time: " << seqTime << " s\n\n";

    std::cout << std::left << std::setw(12) << "Version" << std::setw(9) << "Threads"
            << std::setw(10) << "Run 1" << std::setw(10) << "Run 2" << std::setw(10) << "Run 3"
            << std::setw(10) << "Correct?" << std::setw(14) << "Avg Time (s)" << "Speed-up\n";
    for (int t : {1, 2, 4, 8}) {
        testVersion("Race", countAliveRace, test, correct, seqTime, t);
        testVersion("Critical", countAliveCritical, test, correct, seqTime, t);
        testVersion("Atomic", countAliveAtomic, test, correct, seqTime, t);
        testVersion("Reduction", countAliveReduction, test, correct, seqTime, t);
        std::cout << "\n";
    }

    // ---------------- Experiment 2: single grid vs two grids ----------------
    double gridSeqTotal = 0;
    long long correctAlive = 0;
    for (int r = 0; r < NUM_RUNS; r++)
        gridSeqTotal += runSimulation(SIZE, false, correctAlive);
    double gridSeqTime = gridSeqTotal / NUM_RUNS;

    std::cout << "\n=== Experiment 2: Grid Update ===\n";
    std::cout << "Grid: " << SIZE << "x" << SIZE << ", Generations: " << NUM_GENERATIONS << "\n";
    std::cout << "Correct live cells after " << NUM_GENERATIONS << " generations: " << correctAlive << "\n";
    std::cout << "Sequential time: " << gridSeqTime << " s\n\n";

    std::cout << std::left << std::setw(14) << "Version" << std::setw(9) << "Threads"
            << std::setw(10) << "Run 1" << std::setw(10) << "Run 2" << std::setw(10) << "Run 3"
            << std::setw(10) << "Correct?" << std::setw(14) << "Avg Time (s)" << "Speed-up\n";
    for (int t : {1, 2, 4, 8, 16, 24}) {
        testSimulation("Single grid", true, SIZE, correctAlive, gridSeqTime, t);
        testSimulation("Two grids", false, SIZE, correctAlive, gridSeqTime, t);
        std::cout << "\n";
    }

    return 0;
}