import csv
from collections import defaultdict
import matplotlib.pyplot as plt

csv_file = "task_a_results.csv"
rows = []

with open(csv_file, newline="") as f:
    reader = csv.DictReader(f)
    for row in reader:
        row["n"] = int(row["n"])
        row["avg_ms"] = float(row["avg_ms"])
        row["speedup_vs_seq"] = float(row["speedup_vs_seq"])
        rows.append(row)

by_method = defaultdict(list)
for row in rows:
    by_method[row["method"]].append(row)

for method in by_method:
    by_method[method].sort(key=lambda r: r["n"])

plt.figure(figsize=(8, 5))
for method, method_rows in by_method.items():
    x = [str(r["n"]) for r in method_rows]
    y = [r["avg_ms"] for r in method_rows]
    plt.plot(x, y, marker="o", label=method)

plt.xlabel("Number of elements")
plt.ylabel("Average execution time (ms)")
plt.title("Merge Sort: Sequential vs OpenMP")
plt.legend()
plt.grid(True, linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("task_a_results.png", dpi=150)
print("Saved graph to task_a_results.png")
