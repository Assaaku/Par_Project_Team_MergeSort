#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

using Clock = std::chrono::high_resolution_clock;
using Milliseconds = std::chrono::duration<double, std::milli>;

static constexpr int INSERTION_SORT_LIMIT = 32;

struct BenchmarkResult {
    std::size_t n{};
    std::string method;
    int threads{};
    int trials{};
    double avg_ms{};
    double min_ms{};
    double max_ms{};
    double speedup_vs_seq{};
    double estimated_operations{};
    double host_device_transfer_ms{};
    double host_device_transfer_mb{};
    bool correct{};
};

void insertion_sort(std::vector<int>& a, std::size_t left, std::size_t right) {
    for (std::size_t i = left + 1; i < right; ++i) {
        int key = a[i];
        std::size_t j = i;
        while (j > left && a[j - 1] > key) {
            a[j] = a[j - 1];
            --j;
        }
        a[j] = key;
    }
}

void merge_to_temp(const std::vector<int>& source, std::vector<int>& destination,
                   std::size_t left, std::size_t mid, std::size_t right) {
    std::size_t i = left;
    std::size_t j = mid;
    std::size_t k = left;

    while (i < mid && j < right) {
        if (source[i] <= source[j]) {
            destination[k++] = source[i++];
        } else {
            destination[k++] = source[j++];
        }
    }

    while (i < mid) {
        destination[k++] = source[i++];
    }
    while (j < right) {
        destination[k++] = source[j++];
    }
}

void merge_ranges_in_place(std::vector<int>& a, std::vector<int>& temp,
                           std::size_t left, std::size_t mid, std::size_t right) {
    merge_to_temp(a, temp, left, mid, right);
    for (std::size_t p = left; p < right; ++p) {
        a[p] = temp[p];
    }
}

void merge_sort_sequential_rec(std::vector<int>& a, std::vector<int>& temp,
                               std::size_t left, std::size_t right) {
    std::size_t length = right - left;

    if (length <= 1) {
        return;
    }
    if (length <= INSERTION_SORT_LIMIT) {
        insertion_sort(a, left, right);
        return;
    }

    std::size_t mid = left + length / 2;
    merge_sort_sequential_rec(a, temp, left, mid);
    merge_sort_sequential_rec(a, temp, mid, right);

    // Already ordered: no need to merge-copy.
    if (a[mid - 1] <= a[mid]) {
        return;
    }

    merge_ranges_in_place(a, temp, left, mid, right);
}

void merge_sort_sequential(std::vector<int>& a) {
    std::vector<int> temp(a.size());
    merge_sort_sequential_rec(a, temp, 0, a.size());
}

void merge_sort_openmp(std::vector<int>& a, int requested_threads) {
#ifndef _OPENMP
    (void)a;
    (void)requested_threads;
    std::cerr << "OpenMP is not enabled. Compile with -fopenmp.\n";
    std::exit(1);
#else
    std::size_t n = a.size();
    if (n <= 1) {
        return;
    }

    int threads = std::max(1, requested_threads);
    std::vector<int> temp(n);

    // Bottom-up merge sort:
    // width = size of already-sorted blocks. Each loop merges neighboring blocks.
    for (std::size_t width = 1; width < n; width *= 2) {
        std::size_t step = 2 * width;

        #pragma omp parallel for num_threads(threads) schedule(static)
        for (long long left_signed = 0; left_signed < static_cast<long long>(n); left_signed += static_cast<long long>(step)) {
            std::size_t left = static_cast<std::size_t>(left_signed);
            std::size_t mid = std::min(left + width, n);
            std::size_t right = std::min(left + step, n);

            if (mid >= right) {
                for (std::size_t i = left; i < right; ++i) {
                    temp[i] = a[i];
                }
            } else {
                merge_to_temp(a, temp, left, mid, right);
            }
        }

        a.swap(temp);
    }
#endif
}

std::vector<int> generate_random_data(std::size_t n, unsigned int seed) {
    std::vector<int> data(n);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(-1'000'000, 1'000'000);

    for (std::size_t i = 0; i < n; ++i) {
        data[i] = dist(rng);
    }
    return data;
}

template <typename SortFunction>
double time_sort_ms(const std::vector<int>& input, SortFunction sort_function, bool& correct) {
    std::vector<int> data = input;

    auto start = Clock::now();
    sort_function(data);
    auto end = Clock::now();

    correct = std::is_sorted(data.begin(), data.end());
    return Milliseconds(end - start).count();
}

double average(const std::vector<double>& values) {
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double estimate_merge_sort_operations(std::size_t n) {
    if (n <= 1) {
        return 0.0;
    }
    return static_cast<double>(n) * std::ceil(std::log2(static_cast<double>(n)));
}

BenchmarkResult run_benchmark_for_method(const std::vector<int>& input,
                                         const std::string& method,
                                         int threads,
                                         int trials) {
    std::vector<double> times;
    times.reserve(trials);
    bool all_correct = true;

    for (int trial = 0; trial < trials; ++trial) {
        bool correct = false;
        double elapsed_ms = 0.0;

        if (method == "Sequential") {
            elapsed_ms = time_sort_ms(input, [](std::vector<int>& data) {
                merge_sort_sequential(data);
            }, correct);
        } else if (method == "OpenMP") {
            elapsed_ms = time_sort_ms(input, [threads](std::vector<int>& data) {
                merge_sort_openmp(data, threads);
            }, correct);
        } else {
            std::cerr << "Unknown method: " << method << '\n';
            std::exit(1);
        }

        times.push_back(elapsed_ms);
        all_correct = all_correct && correct;
    }

    auto minmax = std::minmax_element(times.begin(), times.end());

    BenchmarkResult result;
    result.n = input.size();
    result.method = method;
    result.threads = threads;
    result.trials = trials;
    result.avg_ms = average(times);
    result.min_ms = *minmax.first;
    result.max_ms = *minmax.second;
    result.estimated_operations = estimate_merge_sort_operations(input.size());
    result.host_device_transfer_ms = 0.0; // CPU versions do not copy data between host and GPU device.
    result.host_device_transfer_mb = 0.0;
    result.correct = all_correct;
    return result;
}

void print_result_row(const BenchmarkResult& r) {
    std::cout << std::setw(9) << r.n << " | "
              << std::setw(10) << r.method << " | "
              << std::setw(7) << r.threads << " | "
              << std::setw(6) << r.trials << " | "
              << std::setw(10) << std::fixed << std::setprecision(3) << r.avg_ms << " | "
              << std::setw(10) << r.min_ms << " | "
              << std::setw(10) << r.max_ms << " | "
              << std::setw(8) << r.speedup_vs_seq << " | "
              << (r.correct ? "yes" : "no") << '\n';
}

void write_csv_header(std::ofstream& csv) {
    csv << "n,method,threads,trials,avg_ms,min_ms,max_ms,speedup_vs_seq,"
        << "estimated_operations,host_device_transfer_ms,host_device_transfer_mb,correct\n";
}

void write_csv_row(std::ofstream& csv, const BenchmarkResult& r) {
    csv << r.n << ','
        << r.method << ','
        << r.threads << ','
        << r.trials << ','
        << std::fixed << std::setprecision(6) << r.avg_ms << ','
        << r.min_ms << ','
        << r.max_ms << ','
        << r.speedup_vs_seq << ','
        << std::fixed << std::setprecision(0) << r.estimated_operations << ','
        << std::fixed << std::setprecision(6) << r.host_device_transfer_ms << ','
        << r.host_device_transfer_mb << ','
        << (r.correct ? "true" : "false") << '\n';
}

int main(int argc, char* argv[]) {
    int trials = 5;
    int threads = 4;

#ifdef _OPENMP
    threads = omp_get_max_threads();
#endif

    if (argc >= 2) {
        threads = std::max(1, std::atoi(argv[1]));
    }
    if (argc >= 3) {
        trials = std::max(1, std::atoi(argv[2]));
    }

    const std::vector<std::size_t> sizes = {10'000, 100'000, 1'000'000};
    const unsigned int base_seed = 306;

    std::ofstream csv("task_a_results.csv");
    if (!csv) {
        std::cerr << "Could not create task_a_results.csv\n";
        return 1;
    }
    write_csv_header(csv);

    std::cout << "Task A - Merge Sort: Sequential vs OpenMP\n";
    std::cout << "Threads: " << threads << ", trials per method: " << trials << "\n\n";
    std::cout << "        n |     method | threads | trials |     avg ms |     min ms |     max ms |  speedup | correct\n";
    std::cout << "----------+------------+---------+--------+------------+------------+------------+----------+--------\n";

    for (std::size_t n : sizes) {
        std::vector<int> input = generate_random_data(n, base_seed + static_cast<unsigned int>(n));

        BenchmarkResult sequential = run_benchmark_for_method(input, "Sequential", 1, trials);
        sequential.speedup_vs_seq = 1.0;

        BenchmarkResult openmp = run_benchmark_for_method(input, "OpenMP", threads, trials);
        openmp.speedup_vs_seq = sequential.avg_ms / openmp.avg_ms;

        print_result_row(sequential);
        print_result_row(openmp);

        write_csv_row(csv, sequential);
        write_csv_row(csv, openmp);
    }

    std::cout << "\nResults saved to task_a_results.csv\n";
    return 0;
}
