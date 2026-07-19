#!/usr/bin/env python3
"""Compare BENCHMARK_MODE logs for full, runtime-only, and native firmware."""

from __future__ import annotations

import argparse
import math
import re
from pathlib import Path


BENCH_RE = re.compile(r'\[BENCH\]\s+name="([^"]+)"\s+(.+)')


def parse_log(path: Path) -> dict[str, dict[str, int | str]]:
    results: dict[str, dict[str, int | str]] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = BENCH_RE.search(line)
        if not match:
            continue
        values: dict[str, int | str] = {}
        for item in match.group(2).split():
            key, value = item.split("=", 1)
            try:
                values[key] = int(value)
            except ValueError:
                values[key] = value
        results[match.group(1)] = values
    return results


def load_exclusions(path: Path) -> list[re.Pattern[str]]:
    patterns = []
    for line in path.read_text(encoding="utf-8").splitlines():
        value = line.strip()
        if value and not value.startswith("#"):
            patterns.append(re.compile(value))
    return patterns


def coefficient_of_variation(values: dict[str, int | str]) -> float:
    count = int(values.get("n", 0))
    total = int(values.get("sum_cycles", 0))
    total_sq = int(values.get("sumsq_cycles", 0))
    if count < 2 or total == 0:
        return 0.0
    mean = total / count
    variance = max(0.0, total_sq / count - mean * mean)
    return math.sqrt(variance) / mean * 100.0


def ratio(numerator: int, denominator: int) -> float:
    return numerator / denominator if denominator else float("nan")


def geometric_mean(values: list[float]) -> float:
    valid = [value for value in values if value > 0 and math.isfinite(value)]
    return math.exp(sum(math.log(value) for value in valid) / len(valid))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--full", action="append", required=True)
    parser.add_argument("--runtime", action="append", required=True)
    parser.add_argument("--native", action="append", required=True)
    parser.add_argument("--cv-threshold", type=float, default=5.0)
    parser.add_argument(
        "--exclusions",
        default=str(Path(__file__).with_name("evaluation_exclusions.txt")),
    )
    args = parser.parse_args()

    if not (len(args.full) == len(args.runtime) == len(args.native)):
        parser.error("--full, --runtime, and --native counts must match")

    exclusions = load_exclusions(Path(args.exclusions))
    records = []
    for full_name, runtime_name, native_name in zip(
        args.full, args.runtime, args.native
    ):
        full = parse_log(Path(full_name))
        runtime = parse_log(Path(runtime_name))
        native = parse_log(Path(native_name))
        group = Path(full_name).stem
        for name in sorted(set(full) & set(runtime) & set(native)):
            if any(pattern.search(name) for pattern in exclusions):
                continue
            records.append((group, name, full[name], runtime[name], native[name]))

    if not records:
        raise SystemExit("No matching [BENCH] records in the three logs.")

    full_native: list[float] = []
    full_runtime: list[float] = []
    unstable: list[str] = []

    print(
        f"{'Test':<58} {'Full':>10} {'Runtime':>10} {'Native':>10} "
        f"{'F/N':>8} {'F/R':>8} {'CV(full)':>9}"
    )
    for group, name, full_values, runtime_values, native_values in records:
        full_cycles = int(full_values.get("median_cycles", 0))
        runtime_cycles = int(runtime_values.get("median_cycles", 0))
        native_cycles = int(native_values.get("median_cycles", 0))
        fn_ratio = ratio(full_cycles, native_cycles)
        fr_ratio = ratio(full_cycles, runtime_cycles)
        cv = coefficient_of_variation(full_values)
        full_native.append(fn_ratio)
        full_runtime.append(fr_ratio)
        if cv > args.cv_threshold:
            unstable.append(f"{group}: {name}")
        print(
            f"{name:<58} {full_cycles:>10} {runtime_cycles:>10} "
            f"{native_cycles:>10} {fn_ratio:>8.3f} {fr_ratio:>8.3f} {cv:>8.2f}%"
        )

    print(f"\nGeomean Full/Native:       {geometric_mean(full_native):.3f}x")
    print(f"Geomean Full/Runtime-only: {geometric_mean(full_runtime):.3f}x")
    print(f"CV > {args.cv_threshold:.1f}%: {len(unstable)} / {len(records)}")
    for name in unstable:
        print(f"  rerun with BENCHMARK_SAMPLES=1000: {name}")


if __name__ == "__main__":
    main()
