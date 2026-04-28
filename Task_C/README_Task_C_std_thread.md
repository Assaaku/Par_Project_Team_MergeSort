# Task C - Merge Sort using C++ std::thread

This is the Person C implementation for the F.CSM306 Parallel and Distributed Computing assignment.

Person C requirement:

> Use the C++ standard library to manage threads and synchronization, and write the multithreading version.

Algorithm: Merge Sort

Implemented versions in this package:

1. Sequential merge sort baseline
2. Multithreaded merge sort using `std::thread`

The program tests the required input sizes:

- 10,000 elements
- 100,000 elements
- 1,000,000 elements

## Files

- `task_c_thread_merge_sort.cpp` - main C++ source code
- `Makefile_task_c` - compile/run helper
- `plot_task_c_thread.py` - creates comparison graphs from the CSV result
- `Task_C_Report_Notes.md` - explanation and pseudocode for the report

## Compile

```bash
make -f Makefile_task_c
```

Or manually:

```bash
g++ -O2 -std=c++17 -Wall -Wextra -pthread task_c_thread_merge_sort.cpp -o task_c_thread_merge_sort
```

## Run

Default:

```bash
./task_c_thread_merge_sort
```

Recommended with explicit thread count and trials:

```bash
./task_c_thread_merge_sort 8 5
```

Format:

```bash
./task_c_thread_merge_sort <thread_count> <trials>
```

Examples:

```bash
./task_c_thread_merge_sort 4 5
./task_c_thread_merge_sort 8 5
./task_c_thread_merge_sort 16 3
```

## Output

The program prints a result table and writes:

```text
task_c_thread_results.csv
```

The CSV contains:

- input size
- method name
- thread count
- average execution time
- minimum execution time
- maximum execution time
- SpeedUp
- computation time
- data transfer time
- transferred data amount
- estimated operations
- achievable performance
- correctness result

For this CPU-only `std::thread` version, data transfer time is `0`, because data is not copied to a GPU.

## Create graphs

After running the C++ program:

```bash
python3 plot_task_c_thread.py
```

This creates:

```text
task_c_thread_results.png
task_c_thread_speedup.png
```

## Demo video suggestion

For the 30-60 second demo video, show:

1. The source file where `std::thread` is used.
2. Compilation with `make -f Makefile_task_c`.
3. Running the program.
4. Output where `correct = yes` for all input sizes.
5. The generated CSV and graph files.
