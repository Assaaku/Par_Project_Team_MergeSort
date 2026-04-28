import csv
from collections import defaultdict

import matplotlib.pyplot as plt

CSV_FILE = "task_c_thread_results.csv"
TIME_GRAPH = "task_c_thread_results.png"
SPEEDUP_GRAPH = "task_c_thread_speedup.png"

rows = []
with open(CSV_FILE, newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        row["n"] = int(row["n"])
        row["avg_ms"] = float(row["avg_ms"])
        row["speedup"] = float(row["speedup"])
        rows.append(row)

by_method = defaultdict(list)
for row in rows:
    by_method[row["method"]].append(row)

plt.figure(figsize=(8, 5))
for method, method_rows in by_method.items():
    method_rows.sort(key=lambda r: r["n"])
    x = [r["n"] for r in method_rows]
    y = [r["avg_ms"] for r in method_rows]
    plt.plot(x, y, marker="o", label=method)

plt.xscale("log")
plt.yscale("log")
plt.xlabel("Number of elements")
plt.ylabel("Average execution time (ms)")
plt.title("Task C: Merge Sort Sequential vs std::thread")
plt.legend()
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.tight_layout()
plt.savefig(TIME_GRAPH, dpi=200)
print(f"Saved {TIME_GRAPH}")

thread_rows = sorted([r for r in rows if r["method"] == "std_thread"], key=lambda r: r["n"])
plt.figure(figsize=(8, 5))
plt.plot([r["n"] for r in thread_rows], [r["speedup"] for r in thread_rows], marker="o")
plt.xscale("log")
plt.xlabel("Number of elements")
plt.ylabel("SpeedUp compared to sequential")
plt.title("Task C: std::thread Merge Sort SpeedUp")
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.tight_layout()
plt.savefig(SPEEDUP_GRAPH, dpi=200)
print(f"Saved {SPEEDUP_GRAPH}")
