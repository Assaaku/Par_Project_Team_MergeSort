#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/*
 * Task C - Merge Sort using C++ std::thread
 * F.CSM306 Parallel and Distributed Computing Assignment
 *
 * This implementation uses the C++ standard library to manage CPU threads.
 *
 * Main idea:
 *   1. Split the array into several chunks.
 *   2. Sort each chunk in a separate std::thread using normal merge sort.
 *   3. Synchronize using join().
 *   4. Merge neighboring sorted chunks in parallel merge passes.
 *   5. Synchronize after every merge pass using join().
 *
 * Synchronization:
 *   Each thread works on a non-overlapping index range, so a mutex is not needed.
 *   std::thread::join() is used as the synchronization point before moving to the
 *   next merge level.
 */

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

struct TimingStats {
    double average_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
};

struct BenchmarkResult {
    std::size_t n = 0;
    std::string method;
    unsigned int threads = 1;
    int trials = 0;
    double average_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
    double speedup = 1.0;
    double computation_time_ms = 0.0;
    double data_transfer_time_ms = 0.0;
    std::size_t amount_data_transferred_bytes = 0;
    double total_operations = 0.0;
    double achievable_performance_mops = 0.0;
    bool correct = false;
};

struct RunRange {
    std::size_t left;
    std::size_t right;
};

std::vector<int> generate_random_data(std::size_t n, unsigned int seed) {
    std::vector<int> data(n);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 1'000'000);

    for (std::size_t i = 0; i < n; ++i) {
        data[i] = dist(rng);
    }
    return data;
}

NOINLINE void merge_ranges(std::vector<int>& data,
                  std::vector<int>& temp,
                  std::size_t left,
                  std::size_t mid,
                  std::size_t right) {
    std::size_t i = left;
    std::size_t j = mid;
    std::size_t k = left;

    while (i < mid && j < right) {
        if (data[i] <= data[j]) {
            temp[k++] = data[i++];
        } else {
            temp[k++] = data[j++];
        }
    }

    while (i < mid) {
        temp[k++] = data[i++];
    }

    while (j < right) {
        temp[k++] = data[j++];
    }

    for (std::size_t index = left; index < right; ++index) {
        data[index] = temp[index];
    }
}

NOINLINE void sequential_merge_sort_recursive(std::vector<int>& data,
                                     std::vector<int>& temp,
                                     std::size_t left,
                                     std::size_t right) {
    if (right - left <= 1) {
        return;
    }

    const std::size_t mid = left + (right - left) / 2;
    sequential_merge_sort_recursive(data, temp, left, mid);
    sequential_merge_sort_recursive(data, temp, mid, right);
    merge_ranges(data, temp, left, mid, right);
}

NOINLINE void sequential_merge_sort(std::vector<int>& data) {
    std::vector<int> temp(data.size());
    sequential_merge_sort_recursive(data, temp, 0, data.size());
}

unsigned int choose_thread_count(std::size_t n, unsigned int requested_threads) {
    if (requested_threads == 0) {
        requested_threads = 1;
    }

    if (n == 0) {
        return 1;
    }

    const unsigned int n_as_threads = static_cast<unsigned int>(std::min<std::size_t>(n, 1'000'000));
    return std::max(1u, std::min(requested_threads, n_as_threads));
}

std::vector<RunRange> split_into_chunks(std::size_t n, unsigned int thread_count) {
    std::vector<RunRange> chunks;
    chunks.reserve(thread_count);

    const std::size_t base_size = n / thread_count;
    const std::size_t remainder = n % thread_count;

    std::size_t left = 0;
    for (unsigned int i = 0; i < thread_count; ++i) {
        const std::size_t extra = (i < remainder) ? 1 : 0;
        const std::size_t right = left + base_size + extra;
        chunks.push_back({left, right});
        left = right;
    }

    return chunks;
}

NOINLINE void threaded_merge_sort(std::vector<int>& data, unsigned int requested_threads) {
    if (data.size() <= 1) {
        return;
    }

    const unsigned int thread_count = choose_thread_count(data.size(), requested_threads);

    if (thread_count <= 1) {
        sequential_merge_sort(data);
        return;
    }

    std::vector<int> temp(data.size());
    std::vector<RunRange> runs = split_into_chunks(data.size(), thread_count);

    // Step 1: Sort each chunk in a separate thread.
    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    for (const RunRange& range : runs) {
        workers.emplace_back(sequential_merge_sort_recursive,
                             std::ref(data), std::ref(temp), range.left, range.right);
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    // Step 2: Merge sorted chunks. Each pass halves the number of runs.
    while (runs.size() > 1) {
        std::vector<RunRange> next_runs;
        next_runs.reserve((runs.size() + 1) / 2);
        workers.clear();

        for (std::size_t i = 0; i < runs.size(); i += 2) {
            if (i + 1 >= runs.size()) {
                next_runs.push_back(runs[i]);
                continue;
            }

            const std::size_t left = runs[i].left;
            const std::size_t mid = runs[i].right;
            const std::size_t right = runs[i + 1].right;

            workers.emplace_back(merge_ranges,
                                 std::ref(data), std::ref(temp), left, mid, right);
            next_runs.push_back({left, right});
        }

        // Synchronization point for the current merge level.
        for (std::thread& worker : workers) {
            worker.join();
        }

        runs = next_runs;
    }
}

NOINLINE TimingStats compute_stats(const std::vector<double>& times_ms) {
    TimingStats stats;
    if (times_ms.empty()) {
        return stats;
    }

    double sum = 0.0;
    stats.min_ms = std::numeric_limits<double>::max();
    stats.max_ms = std::numeric_limits<double>::lowest();

    for (double t : times_ms) {
        sum += t;
        stats.min_ms = std::min(stats.min_ms, t);
        stats.max_ms = std::max(stats.max_ms, t);
    }

    stats.average_ms = sum / static_cast<double>(times_ms.size());
    return stats;
}

NOINLINE TimingStats measure_sequential(const std::vector<int>& original_data,
                               int trials,
                               bool& all_correct) {
    std::vector<double> times_ms;
    times_ms.reserve(static_cast<std::size_t>(trials));
    all_correct = true;

    for (int trial = 0; trial < trials; ++trial) {
        std::vector<int> data = original_data;

        const auto start = std::chrono::high_resolution_clock::now();
        sequential_merge_sort(data);
        const auto end = std::chrono::high_resolution_clock::now();

        const std::chrono::duration<double, std::milli> elapsed = end - start;
        times_ms.push_back(elapsed.count());

        if (!std::is_sorted(data.begin(), data.end())) {
            all_correct = false;
        }
    }

    return compute_stats(times_ms);
}

NOINLINE TimingStats measure_threaded(const std::vector<int>& original_data,
                             int trials,
                             unsigned int thread_count,
                             bool& all_correct) {
    std::vector<double> times_ms;
    times_ms.reserve(static_cast<std::size_t>(trials));
    all_correct = true;

    for (int trial = 0; trial < trials; ++trial) {
        std::vector<int> data = original_data;

        const auto start = std::chrono::high_resolution_clock::now();
        threaded_merge_sort(data, thread_count);
        const auto end = std::chrono::high_resolution_clock::now();

        const std::chrono::duration<double, std::milli> elapsed = end - start;
        times_ms.push_back(elapsed.count());

        if (!std::is_sorted(data.begin(), data.end())) {
            all_correct = false;
        }
    }

    return compute_stats(times_ms);
}

double estimate_merge_sort_operations(std::size_t n) {
    if (n <= 1) {
        return 0.0;
    }
    return static_cast<double>(n) * std::log2(static_cast<double>(n));
}

double compute_mops(double operations, double time_ms) {
    if (time_ms <= 0.0) {
        return 0.0;
    }
    const double seconds = time_ms / 1000.0;
    return operations / seconds / 1'000'000.0;
}

void write_csv(const std::string& filename, const std::vector<BenchmarkResult>& results) {
    std::ofstream out(filename);
    if (!out) {
        throw std::runtime_error("Could not open CSV file for writing: " + filename);
    }

    out << "n,method,threads,trials,avg_ms,min_ms,max_ms,speedup,"
        << "computation_time_ms,data_transfer_time_ms,amount_data_transferred_bytes,"
        << "total_operations,achievable_performance_mops,correct\n";

    out << std::fixed << std::setprecision(6);
    for (const BenchmarkResult& r : results) {
        out << r.n << ','
            << r.method << ','
            << r.threads << ','
            << r.trials << ','
            << r.average_ms << ','
            << r.min_ms << ','
            << r.max_ms << ','
            << r.speedup << ','
            << r.computation_time_ms << ','
            << r.data_transfer_time_ms << ','
            << r.amount_data_transferred_bytes << ','
            << r.total_operations << ','
            << r.achievable_performance_mops << ','
            << (r.correct ? "yes" : "no") << '\n';
    }
}

void print_results(const std::vector<BenchmarkResult>& results) {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\n"
              << std::setw(10) << "n"
              << " | " << std::setw(13) << "method"
              << " | " << std::setw(7) << "threads"
              << " | " << std::setw(6) << "trials"
              << " | " << std::setw(10) << "avg ms"
              << " | " << std::setw(10) << "min ms"
              << " | " << std::setw(10) << "max ms"
              << " | " << std::setw(8) << "speedup"
              << " | " << "correct"
              << '\n';

    std::cout << std::string(103, '-') << '\n';

    for (const BenchmarkResult& r : results) {
        std::cout << std::setw(10) << r.n
                  << " | " << std::setw(13) << r.method
                  << " | " << std::setw(7) << r.threads
                  << " | " << std::setw(6) << r.trials
                  << " | " << std::setw(10) << r.average_ms
                  << " | " << std::setw(10) << r.min_ms
                  << " | " << std::setw(10) << r.max_ms
                  << " | " << std::setw(8) << r.speedup
                  << " | " << (r.correct ? "yes" : "no")
                  << '\n';
    }
}

int main(int argc, char* argv[]) {
    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) {
        thread_count = 4;
    }

    int trials = 5;

    if (argc >= 2) {
        thread_count = static_cast<unsigned int>(std::stoul(argv[1]));
        thread_count = std::max(1u, thread_count);
    }

    if (argc >= 3) {
        trials = std::stoi(argv[2]);
        trials = std::max(1, trials);
    }

    const std::vector<std::size_t> sizes = {10'000, 100'000, 1'000'000};
    std::vector<BenchmarkResult> results;

    std::cout << "Task C - Merge Sort: Sequential vs std::thread\n";
    std::cout << "Requested threads: " << thread_count << "\n";
    std::cout << "Trials per method: " << trials << "\n";

    for (std::size_t n : sizes) {
        const std::vector<int> original_data = generate_random_data(n, 12345u + static_cast<unsigned int>(n));
        const double operations = estimate_merge_sort_operations(n);
        const unsigned int actual_thread_count = choose_thread_count(n, thread_count);

        bool sequential_correct = false;
        const TimingStats sequential_stats = measure_sequential(original_data, trials, sequential_correct);

        bool threaded_correct = false;
        const TimingStats threaded_stats = measure_threaded(original_data, trials, actual_thread_count, threaded_correct);

        BenchmarkResult sequential_result;
        sequential_result.n = n;
        sequential_result.method = "Sequential";
        sequential_result.threads = 1;
        sequential_result.trials = trials;
        sequential_result.average_ms = sequential_stats.average_ms;
        sequential_result.min_ms = sequential_stats.min_ms;
        sequential_result.max_ms = sequential_stats.max_ms;
        sequential_result.speedup = 1.0;
        sequential_result.computation_time_ms = sequential_stats.average_ms;
        sequential_result.data_transfer_time_ms = 0.0;
        sequential_result.amount_data_transferred_bytes = 0;
        sequential_result.total_operations = operations;
        sequential_result.achievable_performance_mops = compute_mops(operations, sequential_stats.average_ms);
        sequential_result.correct = sequential_correct;
        results.push_back(sequential_result);

        BenchmarkResult threaded_result;
        threaded_result.n = n;
        threaded_result.method = "std_thread";
        threaded_result.threads = actual_thread_count;
        threaded_result.trials = trials;
        threaded_result.average_ms = threaded_stats.average_ms;
        threaded_result.min_ms = threaded_stats.min_ms;
        threaded_result.max_ms = threaded_stats.max_ms;
        threaded_result.speedup = sequential_stats.average_ms / threaded_stats.average_ms;
        threaded_result.computation_time_ms = threaded_stats.average_ms;
        threaded_result.data_transfer_time_ms = 0.0;
        threaded_result.amount_data_transferred_bytes = 0;
        threaded_result.total_operations = operations;
        threaded_result.achievable_performance_mops = compute_mops(operations, threaded_stats.average_ms);
        threaded_result.correct = threaded_correct;
        results.push_back(threaded_result);
    }

    print_results(results);

    const std::string csv_filename = "task_c_thread_results.csv";
    write_csv(csv_filename, results);
    std::cout << "\nCSV written to: " << csv_filename << "\n";

    std::cout << "\nNote: data_transfer_time_ms and amount_data_transferred_bytes are 0 for std::thread,\n"
              << "because this is a CPU-only version and does not transfer data to a GPU.\n";

    return 0;
}
