#!/usr/bin/env python3
"""Attribute detector results against runtime-only and native baselines."""

from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass
from pathlib import Path


DETECTOR_SOURCES = {"software-tag", "null", "integer", "mpu"}


@dataclass
class Observation:
    verdict: str = "UNKNOWN"
    bad_source: str = "unknown"
    good_source: str = "unknown"


def parse_log(path: Path) -> dict[str, Observation]:
    results: dict[str, Observation] = {}
    current: Observation | None = None

    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = re.search(
            r"\[(PASS|FAIL(?:-MISS|-FP|-BOTH)?)\s*\]\s+(.*)", line
        )
        if match:
            current = Observation(verdict=match.group(1))
            results[match.group(2).strip()] = current
            continue

        match = re.search(
            r"\[DETECT\]\s+bad_source=(\S+)\s+good_source=(\S+)", line
        )
        if match and current is not None:
            current.bad_source = match.group(1)
            current.good_source = match.group(2)

    return results


def load_exclusions(path: Path) -> list[re.Pattern[str]]:
    patterns = []
    for line in path.read_text(encoding="utf-8").splitlines():
        value = line.strip()
        if value and not value.startswith("#"):
            patterns.append(re.compile(value))
    return patterns


def classify(
    passed: Observation,
    runtime: Observation,
    native: Observation | None,
) -> str:
    if passed.good_source != "none":
        return "FALSE-POSITIVE"
    if runtime.good_source not in {"none", "unknown"}:
        return "BASELINE-GOOD-FAULT"
    if native and native.good_source not in {"none", "unknown"}:
        return "BASELINE-GOOD-FAULT"

    runtime_detected = runtime.bad_source not in {"none", "unknown"}
    native_detected = (
        native is not None and native.bad_source not in {"none", "unknown"}
    )
    if runtime_detected or native_detected:
        return "BASELINE-DETECTED"
    if passed.bad_source in DETECTOR_SOURCES:
        return "ATTRIBUTED-PASS"
    if passed.bad_source == "cpu-fault":
        return "CPU-FAULT-ONLY"
    return "MISS"


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Count only pass-exclusive, detector-sourced detections as success."
    )
    parser.add_argument("--pass-log", action="append", required=True)
    parser.add_argument("--runtime-log", action="append", required=True)
    parser.add_argument("--native-log", action="append", default=[])
    parser.add_argument(
        "--exclusions",
        default=str(Path(__file__).with_name("evaluation_exclusions.txt")),
    )
    parser.add_argument("--csv", help="Optional detailed CSV output")
    args = parser.parse_args()

    if len(args.pass_log) != len(args.runtime_log):
        parser.error("--pass-log and --runtime-log counts must match")
    if args.native_log and len(args.native_log) != len(args.pass_log):
        parser.error("--native-log count must be zero or match --pass-log")

    exclusions = load_exclusions(Path(args.exclusions))
    rows: list[dict[str, str]] = []

    for index, (pass_name, runtime_name) in enumerate(
        zip(args.pass_log, args.runtime_log)
    ):
        pass_path = Path(pass_name)
        runtime_path = Path(runtime_name)
        native_path = Path(args.native_log[index]) if args.native_log else None
        pass_results = parse_log(pass_path)
        runtime_results = parse_log(runtime_path)
        native_results = parse_log(native_path) if native_path else {}

        for name, passed in pass_results.items():
            if any(pattern.search(name) for pattern in exclusions):
                continue
            runtime = runtime_results.get(name, Observation())
            native = native_results.get(name) if native_path else None
            status = classify(passed, runtime, native)
            rows.append(
                {
                    "group": pass_path.stem,
                    "test": name,
                    "status": status,
                    "pass_source": passed.bad_source,
                    "runtime_source": runtime.bad_source,
                    "native_source": native.bad_source if native else "not-run",
                    "pass_good_source": passed.good_source,
                }
            )

    unknown = [
        row
        for row in rows
        if "unknown"
        in {
            row["pass_source"],
            row["runtime_source"],
            row["pass_good_source"],
        }
    ]
    if unknown:
        raise SystemExit(
            "Logs without [DETECT] attribution were supplied; rebuild and rerun them."
        )

    counts: dict[str, int] = {}
    for row in rows:
        counts[row["status"]] = counts.get(row["status"], 0) + 1

    success = counts.get("ATTRIBUTED-PASS", 0)
    total = len(rows)
    rate = success / total * 100.0 if total else 0.0
    print(f"Attributed result: {success} / {total} ({rate:.1f}%)")
    for status in sorted(counts):
        print(f"  {status:<22} {counts[status]:>4}")

    if args.csv:
        with Path(args.csv).open("w", newline="", encoding="utf-8") as output:
            writer = csv.DictWriter(output, fieldnames=rows[0].keys() if rows else [])
            if rows:
                writer.writeheader()
                writer.writerows(rows)


if __name__ == "__main__":
    main()
