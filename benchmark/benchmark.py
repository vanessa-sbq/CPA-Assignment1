#!/usr/bin/env python3
import subprocess
import re
import argparse
import os
from openpyxl import Workbook, load_workbook

BINARY      = "./Assignment1"
EXCEL_FILE  = "benchmark/results.xlsx"
PERF_EVENTS = "L1-dcache-load-misses,LLC-load-misses,cycles"

SIZES = {
    1: [1024, 1536, 2048, 2560, 3072],
    2: [1024, 1536, 2048, 2560, 3072, 4096, 6144, 8192, 10240],
    3: [8192],
    4: [8192],
}
THREAD_COUNTS = list(reversed([4, 8, 12, 16, 20, 24]))

# Row indices (starting on 1, because of openpyxl)
ROWS = {"time": 2, "gflops": 3, "l1_miss": 4, "l2_miss": 5, "cycles": 6, "joules": 7, "watts": 8}


def make_input(op, size, num_threads=None):
    parts = [op, f"{size} {size}"]
    if num_threads is not None:
        parts.append(num_threads)
    parts.append(0)
    return "\n".join(str(p) for p in parts) + "\n"


def run_benchmark(op, size, num_threads=None):
    """Run benchmark with perf stat; falls back silently if perf is unavailable."""
    input_data = make_input(op, size, num_threads)
    cmd = ["perf", "stat", "-e", PERF_EVENTS, BINARY]

    try:
        result = subprocess.run(cmd, input=input_data, capture_output=True, text=True, timeout=600)
        perf_output = result.stderr
    except FileNotFoundError: # perf not found, run without it
        result = subprocess.run([BINARY], input=input_data, capture_output=True, text=True, timeout=600)
        perf_output = ""

    stdout = result.stdout
    time_match   = re.search(r"Time:\s*([\d.eE+-]+)\s*seconds", stdout)
    gflops_match = re.search(r"GFlop/s:\s*([\d.eE+-]+)", stdout)
    joules_match = re.search(r"Joules:\s*([\d.eE+-]+)", stdout)
    watts_match  = re.search(r"Watts:\s*([\d.eE+-]+)", stdout)

    if not time_match:
        raise ValueError(f"Could not parse time from output:\n{stdout}")

    def parse_perf(name):
        m = re.search(rf"([\d,]+)\s+{re.escape(name)}", perf_output)
        return int(m.group(1).replace(",", "")) if m else None

    return {
        "time":    float(time_match.group(1)),
        "gflops":  float(gflops_match.group(1)) if gflops_match else None,
        "joules":  float(joules_match.group(1)) if joules_match else None,
        "watts":  float(watts_match.group(1))  if watts_match  else None,
        "l1_miss": parse_perf("L1-dcache-load-misses"),
        "l2_miss": parse_perf("LLC-load-misses"),
        "cycles":  parse_perf("cycles"),
    }


def get_or_create_sheet(wb, name, col_headers):
    if name in wb.sheetnames:
        return wb[name]
    ws = wb.create_sheet(name)
    ws.append(["Metric"]    + col_headers)
    ws.append(["Time (s)"]  + [None] * len(col_headers))
    ws.append(["GFlop/s"]   + [None] * len(col_headers))
    ws.append(["L1 Misses"] + [None] * len(col_headers))
    ws.append(["L2 Misses"] + [None] * len(col_headers))
    ws.append(["Cycles"]    + [None] * len(col_headers))
    ws.append(["Joules"]    + [None] * len(col_headers))
    ws.append(["Watts"]  + [None] * len(col_headers))
    return ws


def get_col(keys, key):
    return keys.index(key) + 2


def read_cell(ws, keys, key, row):
    return ws.cell(row=row, column=get_col(keys, key)).value


def write_data(ws, keys, key, data):
    col = get_col(keys, key)
    for metric, row in ROWS.items():
        if data.get(metric) is not None:
            ws.cell(row=row, column=col, value=data[metric])


def run_sequential(wb, op, sheet_name):
    sizes = SIZES[op]
    ws = get_or_create_sheet(wb, sheet_name, [f"{s}x{s}" for s in sizes])

    for size in sizes:
        if read_cell(ws, sizes, size, ROWS["time"]) is not None:
            print(f"  [SKIP] {size}x{size}")
            continue
        print(f"  [RUN]  {size}x{size}...", end=" ", flush=True)
        try:
            data = run_benchmark(op, size)
            write_data(ws, sizes, size, data)
            wb.save(EXCEL_FILE)
            print(f"{data['time']:.3f}s | L1: {data['l1_miss'] or 'N/A'} | L2: {data['l2_miss'] or 'N/A'} | {data['joules'] or 'N/A'}J  (saved)")
        except Exception as e:
            print(f"FAILED: {e}")
            break


def run_parallel(wb, op, sheet_name):
    size = SIZES[op][0]
    ws = get_or_create_sheet(wb, sheet_name, [f"{t} threads" for t in THREAD_COUNTS])

    for num_threads in THREAD_COUNTS:
        if read_cell(ws, THREAD_COUNTS, num_threads, ROWS["time"]) is not None:
            print(f"  [SKIP] {num_threads} threads")
            continue
        print(f"  [RUN]  {num_threads} threads...", end=" ", flush=True)
        try:
            data = run_benchmark(op, size, num_threads)
            write_data(ws, THREAD_COUNTS, num_threads, data)
            wb.save(EXCEL_FILE)
            print(f"{data['time']:.3f}s | {data['gflops']:.2f} GFlop/s | L1: {data['l1_miss'] or 'N/A'} | {data['joules'] or 'N/A'}J  (saved)")
        except Exception as e:
            print(f"FAILED: {e}")
            break


def main():
    parser = argparse.ArgumentParser(description="Run mmult benchmarks and save to Excel.")
    parser.add_argument("--op", type=int, choices=[1, 2, 3, 4], help="1=OnMult, 2=OnMultLine, 3=OnMultLineParallel, 4=OnMultLineParallelSIMD")
    args = parser.parse_args()

    ops_info = {
        1: ("OnMult",                 False),
        2: ("OnMultLine",             False),
        3: ("OnMultLineParallel",     True),
        4: ("OnMultLineParallelSIMD", True),
    }

    wb = load_workbook(EXCEL_FILE) if os.path.exists(EXCEL_FILE) else Workbook()
    if "Sheet" in wb.sheetnames: # Remove default sheet if it exists
        del wb["Sheet"]

    for op, (sheet_name, is_parallel) in ops_info.items():
        if args.op is not None and op != args.op:
            continue
        print(f"\nBenchmarking {sheet_name}...")
        (run_parallel if is_parallel else run_sequential)(wb, op, sheet_name)

    print(f"\nDone. Results saved to {EXCEL_FILE}")


if __name__ == "__main__":
    main()
