#!/usr/bin/env python3
"""
compare_metrics.py — PASS firmware vs NO_PASS firmware 성능 비교

사용법:
  # UART 로그 비교 (보드에서 수집한 로그)
  python3 compare_metrics.py --pass-log with_pass.log --nopass-log no_pass.log

  # 정적 바이너리 크기만 비교 (보드 없이)
  python3 compare_metrics.py --pass-elf firmware/firmware_CWE121_s01.elf \
                              --nopass-elf firmware/firmware_CWE121_s01_nopass.elf

  # 둘 다
  python3 compare_metrics.py --pass-log with_pass.log --nopass-log no_pass.log \
                              --pass-elf firmware/firmware_CWE121_s01.elf \
                              --nopass-elf firmware/firmware_CWE121_s01_nopass.elf

  # firmware/ 디렉토리 전체 자동 매칭
  python3 compare_metrics.py --firmware-dir firmware/
"""

from __future__ import annotations

import re
import sys
import argparse
import subprocess
from pathlib import Path


# ── 로그 파싱 ─────────────────────────────────────────────────────────────────

def parse_log(path: str) -> dict:
    """
    UART 로그에서 [METRIC] 라인을 파싱한다.
    반환: { test_name: {bad_us, good_us, bad_heap, good_heap, bad_stk, good_stk} }
    """
    results = {}
    current_name = None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.search(
                r'\[(PASS|FAIL(?:-MISS|-FP|-BOTH)?)\s*\]\s+(.*)',
                line,
            )
            if m:
                current_name = m.group(2).strip()

            m = re.search(r'\[METRIC\]\s+(.+)', line)
            if m and current_name:
                try:
                    kv = dict(item.split('=') for item in m.group(1).split())
                    results[current_name] = {k: int(v) for k, v in kv.items()}
                except ValueError:
                    pass
    return results


# ── ELF 크기 파싱 ─────────────────────────────────────────────────────────────

def elf_size(elf_path: str) -> dict | None:
    """arm-none-eabi-size 로 text/data/bss 파싱"""
    try:
        out = subprocess.check_output(
            ["arm-none-eabi-size", elf_path], text=True, stderr=subprocess.DEVNULL
        )
        lines = out.strip().splitlines()
        if len(lines) < 2:
            return None
        parts = lines[-1].split()
        text, data, bss = int(parts[0]), int(parts[1]), int(parts[2])
        return {
            "flash": text + data,
            "ram_static": data + bss,
            "text": text,
            "data": data,
            "bss": bss,
        }
    except Exception:
        return None


# ── 동적 지표 비교 출력 ────────────────────────────────────────────────────────

def print_dynamic_comparison(pass_data: dict, nopass_data: dict):
    all_names = sorted(set(list(pass_data.keys()) + list(nopass_data.keys())))
    if not all_names:
        print("  (비교할 테스트케이스 없음)")
        return

    col = max(len(n) for n in all_names)
    col = max(col, 40)

    header = (
        f"{'Test Case':<{col}}  "
        f"{'bad_us':>8} {'(nopass)':>8} {'오버헤드':>8}  "
        f"{'heap_pass':>9} {'heap_nopass':>11}  "
        f"{'stk_pass':>8} {'stk_nopass':>10}"
    )
    print(header)
    print("─" * len(header))

    total_p, total_n = 0, 0
    for name in all_names:
        pd = pass_data.get(name, {})
        nd = nopass_data.get(name, {})

        bad_p = pd.get("bad_us", 0)
        bad_n = nd.get("bad_us", 0)
        overhead = f"{(bad_p - bad_n) / bad_n * 100:+.1f}%" if bad_n > 0 else "  N/A"

        heap_p = pd.get("bad_heap", 0)
        heap_n = nd.get("bad_heap", 0)
        stk_p  = pd.get("bad_stk", 0)
        stk_n  = nd.get("bad_stk", 0)

        total_p += bad_p
        total_n += bad_n

        print(
            f"{name:<{col}}  "
            f"{bad_p:>8} {bad_n:>8} {overhead:>8}  "
            f"{heap_p:>7}B  {heap_n:>9}B  "
            f"{stk_p:>6}B  {stk_n:>8}B"
        )

    print("─" * len(header))
    total_oh = f"{(total_p - total_n) / total_n * 100:+.1f}%" if total_n > 0 else "N/A"
    print(
        f"{'TOTAL / 평균':<{col}}  "
        f"{total_p:>8} {total_n:>8} {total_oh:>8}"
    )


# ── 정적 크기 비교 출력 ────────────────────────────────────────────────────────

def print_size_comparison(pass_elf: str, nopass_elf: str, label: str = ""):
    ps = elf_size(pass_elf)
    ns = elf_size(nopass_elf)
    if not ps or not ns:
        print(f"  [경고] ELF 크기를 읽을 수 없습니다.")
        return

    tag = f" ({label})" if label else ""
    print(f"\n{'':=<60}")
    print(f"  정적 메모리 비교{tag}")
    print(f"{'':=<60}")
    rows = [
        ("Flash (text+data)", ps["flash"], ns["flash"]),
        ("  text",            ps["text"],  ns["text"]),
        ("  data",            ps["data"],  ns["data"]),
        ("RAM 정적 (data+bss)", ps["ram_static"], ns["ram_static"]),
        ("  bss",             ps["bss"],   ns["bss"]),
    ]
    for name, pv, nv in rows:
        diff = pv - nv
        pct  = diff / nv * 100 if nv > 0 else 0.0
        print(f"  {name:<24}  pass={pv:>8}B  nopass={nv:>8}B  diff={diff:>+8}B ({pct:+.1f}%)")


# ── firmware/ 디렉토리 자동 매칭 ──────────────────────────────────────────────

def auto_compare_dir(firmware_dir: str):
    p = Path(firmware_dir)
    elfs = list(p.glob("*.elf"))
    nopass = {f.stem.replace("_nopass", ""): f for f in elfs if "_nopass" in f.name}
    with_pass = {f.stem: f for f in elfs if "_nopass" not in f.name}

    common = sorted(set(nopass.keys()) & set(with_pass.keys()))
    if not common:
        print("매칭되는 (pass / nopass) ELF 쌍이 없습니다.")
        return

    for label in common:
        print_size_comparison(str(with_pass[label]), str(nopass[label]), label=label)


# ── 메인 ──────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="PASS vs NO_PASS 성능 비교")
    ap.add_argument("--pass-log",     help="패스 적용 firmware UART 로그")
    ap.add_argument("--nopass-log",   help="패스 미적용 firmware UART 로그")
    ap.add_argument("--pass-elf",     help="패스 적용 ELF 파일")
    ap.add_argument("--nopass-elf",   help="패스 미적용 ELF 파일")
    ap.add_argument("--firmware-dir", help="firmware/ 디렉토리 자동 매칭")
    args = ap.parse_args()

    if not any(vars(args).values()):
        ap.print_help()
        sys.exit(0)

    # 동적 지표 (UART 로그)
    if args.pass_log and args.nopass_log:
        pass_data   = parse_log(args.pass_log)
        nopass_data = parse_log(args.nopass_log)
        print(f"\n{'':=<60}")
        print("  동적 지표 비교 (실행시간 / 힙 / 스택)")
        print(f"{'':=<60}")
        print_dynamic_comparison(pass_data, nopass_data)

    # 정적 크기 (단일 ELF 쌍)
    if args.pass_elf and args.nopass_elf:
        print_size_comparison(args.pass_elf, args.nopass_elf)

    # 정적 크기 (디렉토리 자동 매칭)
    if args.firmware_dir:
        auto_compare_dir(args.firmware_dir)


if __name__ == "__main__":
    main()
