# Task A - Merge Sort: Sequential and OpenMP

This part implements the assignment member A responsibility:

- Sequential merge sort
- OpenMP parallel merge sort
- Tests on 10k, 100k, and 1M elements
- Measures average, minimum, maximum execution time
- Calculates OpenMP speedup compared with sequential
- Writes results to `task_a_results.csv`
- Optional graph generation with `plot_task_a.py`

## Build

```bash
make
```

Or directly:

```bash
g++ -O2 -std=c++17 -Wall -Wextra -fopenmp task_a_merge_sort.cpp -o task_a_merge_sort
```

## Run

```bash
./task_a_merge_sort
```

Optional arguments:

```bash
./task_a_merge_sort <threads> <trials>
```

Example:

```bash
./task_a_merge_sort 8 5
```

## Plot

After running the C++ program:

```bash
python3 plot_task_a.py
```

This creates `task_a_results.png`.
