#!/usr/bin/env python3
"""
Download all Juliet test case .c files from ispras/juliet-dynamic for the
CWEs covered by this project, then auto-generate test_runner.c and
test_cases_testcases.h from the downloaded files.

Usage:
    python3 download_and_generate.py
"""

import os
import re
import sys
import json
import time
import urllib.request
import urllib.error

# ── Configuration ──────────────────────────────────────────────────────────

BASE_URL  = "https://api.github.com/repos/ispras/juliet-dynamic/contents/testcases"
RAW_URL   = "https://raw.githubusercontent.com/ispras/juliet-dynamic/master/testcases"
PROJ_ROOT = os.path.dirname(os.path.abspath(__file__))
TC_DIR    = os.path.join(PROJ_ROOT, "stm32", "Core", "Src", "testcases")

# (CWE-dir, subdir) pairs to download — skip network/C++ subdirs
TARGETS = [
    ("CWE121_Stack_Based_Buffer_Overflow",  "s02"),
    ("CWE121_Stack_Based_Buffer_Overflow",  "s03"),
    ("CWE121_Stack_Based_Buffer_Overflow",  "s04"),
    ("CWE121_Stack_Based_Buffer_Overflow",  "s05"),
    ("CWE121_Stack_Based_Buffer_Overflow",  "s06"),
    ("CWE121_Stack_Based_Buffer_Overflow",  "s07"),
    ("CWE121_Stack_Based_Buffer_Overflow",  "s08"),
    ("CWE121_Stack_Based_Buffer_Overflow",  "s09"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s01"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s05"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s06"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s07"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s08"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s09"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s10"),
    ("CWE122_Heap_Based_Buffer_Overflow",   "s11"),
    ("CWE124_Buffer_Underwrite",            "s02"),
    ("CWE124_Buffer_Underwrite",            "s03"),
    ("CWE124_Buffer_Underwrite",            "s04"),
    ("CWE126_Buffer_Overread",              "s02"),
    ("CWE415_Double_Free",                  "s01"),
    # flat dirs (no subdir)
    ("CWE416_Use_After_Free",               None),
    ("CWE476_NULL_Pointer_Dereference",     None),
]

# ── Helpers ─────────────────────────────────────────────────────────────────

def api_get(url):
    req = urllib.request.Request(url, headers={"Accept": "application/vnd.github+json"})
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            return json.loads(r.read().decode())
    except urllib.error.HTTPError as e:
        if e.code == 403:
            print("  [rate-limited] sleeping 60s …", flush=True)
            time.sleep(60)
            return api_get(url)
        raise

def download_file(url, dest):
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req, timeout=30) as r:
        with open(dest, "wb") as f:
            f.write(r.read())

def list_c_files(cwe_dir, subdir):
    path = f"{cwe_dir}/{subdir}" if subdir else cwe_dir
    url  = f"{BASE_URL}/{path}"
    entries = api_get(url)
    return [e["name"] for e in entries if e["type"] == "file" and e["name"].endswith(".c")]

def download_dir(cwe_dir, subdir):
    local_dir = os.path.join(TC_DIR, cwe_dir, subdir) if subdir else os.path.join(TC_DIR, cwe_dir)
    os.makedirs(local_dir, exist_ok=True)

    files = list_c_files(cwe_dir, subdir)
    new = 0
    for fname in files:
        dest = os.path.join(local_dir, fname)
        if os.path.exists(dest):
            continue
        rel = f"{cwe_dir}/{subdir}/{fname}" if subdir else f"{cwe_dir}/{fname}"
        url = f"{RAW_URL}/{rel}"
        try:
            download_file(url, dest)
            new += 1
        except Exception as e:
            print(f"  FAIL {fname}: {e}", flush=True)
    return files, new

# ── Detect primary test files ────────────────────────────────────────────────
# A primary file defines the top-level _bad() function (no arguments).

BAD_RE = re.compile(r'^void\s+(CWE\w+_\d+(?:[a-z])?_bad)\(\s*\)', re.MULTILINE)

def find_primary_functions(filepath):
    """Return (bad_fn, good_fn) if this file is a primary test file, else None."""
    with open(filepath, encoding="utf-8", errors="replace") as f:
        src = f.read()
    m = BAD_RE.search(src)
    if not m:
        return None
    bad_fn = m.group(1)
    good_fn = bad_fn[:-4] + "_good"   # replace _bad → _good
    # verify good function exists
    if f"void {good_fn}" not in src:
        return None
    return bad_fn, good_fn

# ── Scan all downloaded test cases ──────────────────────────────────────────

def scan_testcases():
    """Return list of (label, bad_fn, good_fn) for every primary test file."""
    entries = []
    for cwe_dir, subdir in TARGETS:
        local_dir = os.path.join(TC_DIR, cwe_dir, subdir) if subdir else os.path.join(TC_DIR, cwe_dir)
        if not os.path.isdir(local_dir):
            continue
        for fname in sorted(os.listdir(local_dir)):
            if not fname.endswith(".c"):
                continue
            fpath = os.path.join(local_dir, fname)
            result = find_primary_functions(fpath)
            if result is None:
                continue
            bad_fn, good_fn = result
            # derive a short display label
            stem   = fname[:-2]   # strip .c
            # e.g. CWE121_Stack_Based_Buffer_Overflow__CWE193_char_alloca_loop_01
            # → CWE121 (CWE193_char_alloca_loop_01)
            cwe_num = re.match(r'(CWE\d+)', stem).group(1)
            suffix  = stem.split("__", 1)[1] if "__" in stem else stem
            label   = f"{cwe_num} ({suffix})"
            entries.append((label, bad_fn, good_fn))
    return entries

# ── Generate test_runner.c ────────────────────────────────────────────────────

RUNNER_TEMPLATE = """\
#include "test_runner.h"
#include "main.h"
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ── shared recovery state ───────────────────────────────────────────── */
jmp_buf      g_test_recovery;
volatile int g_test_running   = 0;
volatile int g_error_detected = 0;

/* ── DWT cycle counter ───────────────────────────────────────────────── */
#define DEM_CR     (*(volatile uint32_t *)0xE000EDFC)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004)

static void dwt_init(void) {{
    DEM_CR   |= (1UL << 24);
    DWT_CTRL |= (1UL << 0);
}}

static uint32_t dwt_elapsed_us(uint32_t start) {{
    return (DWT_CYCCNT - start) / (SystemCoreClock / 1000000U);
}}

/* ── state reset between test cases ─────────────────────────────────── */
extern void HAL_MPU_Disable(void);
extern void HAL_MPU_Enable(uint32_t MPU_Control);
extern void configure_mpu_for_null_ptr(void);
extern void heap_reset(void);
extern void tags_reset(void);

#define MPU_RNR          (*(volatile uint32_t *)0xE000ED98)
#define MPU_RLAR         (*(volatile uint32_t *)0xE000EDA0)
#define MPU_PRIVILEGED_DEFAULT  4U

static void reset_mpu(void) {{
    HAL_MPU_Disable();
    for (int i = 0; i < 7; i++) {{
        MPU_RNR  = (uint32_t)i;
        MPU_RLAR &= ~0x1UL;
    }}
    configure_mpu_for_null_ptr();
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}}

static void reset_test_state(void) {{
    reset_mpu();
    heap_reset();
    tags_reset();
}}

/* ── UART output ─────────────────────────────────────────────────────── */
extern void uart_send_string(const char *str);

/* ── test-case table ─────────────────────────────────────────────────── */
typedef struct {{
    const char *name;
    void (*bad)(void);
    void (*good)(void);
}} TestCase;

/* forward declarations */
{forward_decls}

static const TestCase test_table[] = {{
{table_entries}
}};

#define NUM_TESTS  (sizeof(test_table) / sizeof(test_table[0]))

/* ── runner ──────────────────────────────────────────────────────────── */
void run_all_tests(void) {{
    dwt_init();

    char buf[160];
    int  pass = 0, fail = 0;

    uart_send_string("\\r\\n========================================\\r\\n");
    uart_send_string("  Memory Error Detection Test Runner\\r\\n");
    uart_send_string("========================================\\r\\n");

    for (int i = 0; i < (int)NUM_TESTS; i++) {{
        const TestCase *tc = &test_table[i];

        /* ---- bad path: must detect ---- */
        reset_test_state();
        g_error_detected = 0;
        g_test_running   = 1;
        uint32_t t0 = DWT_CYCCNT;
        if (setjmp(g_test_recovery) == 0) {{
            tc->bad();
        }}
        uint32_t t_bad_us = dwt_elapsed_us(t0);
        int bad_caught = g_error_detected;
        g_test_running = 0;

        /* ---- good path: must NOT detect ---- */
        reset_test_state();
        g_error_detected = 0;
        g_test_running   = 1;
        t0 = DWT_CYCCNT;
        if (setjmp(g_test_recovery) == 0) {{
            tc->good();
        }}
        uint32_t t_good_us = dwt_elapsed_us(t0);
        int good_fp = g_error_detected;
        g_test_running = 0;

        const char *verdict;
        if      ( bad_caught && !good_fp) {{ verdict = "PASS    "; pass++; }}
        else if (!bad_caught && !good_fp) {{ verdict = "FAIL-MISS"; fail++; }}
        else if ( bad_caught &&  good_fp) {{ verdict = "FAIL-FP  "; fail++; }}
        else                              {{ verdict = "FAIL-BOTH"; fail++; }}

        snprintf(buf, sizeof(buf),
                 "[%s] %s\\r\\n"
                 "         bad: %6lu us  good: %6lu us\\r\\n",
                 verdict, tc->name,
                 (unsigned long)t_bad_us,
                 (unsigned long)t_good_us);
        uart_send_string(buf);
    }}

    uart_send_string("----------------------------------------\\r\\n");
    snprintf(buf, sizeof(buf),
             "  Result: %d / %d PASS\\r\\n", pass, (int)NUM_TESTS);
    uart_send_string(buf);
    uart_send_string("========================================\\r\\n");
}}
"""

def generate_test_runner(entries):
    fwd_lines   = []
    table_lines = []
    for label, bad_fn, good_fn in entries:
        fwd_lines.append(f"void {bad_fn}(void);")
        fwd_lines.append(f"void {good_fn}(void);")
        # escape quotes in label (shouldn't be any, but be safe)
        safe_label = label.replace('"', '\\"')
        table_lines.append(
            f'    {{ "{safe_label}",\n'
            f'      {bad_fn},\n'
            f'      {good_fn} }},'
        )
    fwd_block   = "\n".join(fwd_lines)
    table_block = "\n".join(table_lines)
    return RUNNER_TEMPLATE.format(forward_decls=fwd_block, table_entries=table_block)

# ── Generate test_cases_testcases.h ─────────────────────────────────────────

def generate_header(entries):
    lines = ["#ifndef TEST_CASES_H", "#define TEST_CASES_H", ""]
    prev_cwe = None
    for label, bad_fn, good_fn in entries:
        cwe = re.match(r'(CWE\d+)', label).group(1)
        if cwe != prev_cwe:
            lines.append(f"/* ── {cwe} ── */")
            prev_cwe = cwe
        lines.append(f"void {bad_fn}(void);")
        lines.append(f"void {good_fn}(void);")
    lines += ["", "#endif"]
    return "\n".join(lines) + "\n"

# ── Main ─────────────────────────────────────────────────────────────────────

def main():
    print("=== Downloading test case files ===")
    total_new = 0
    for cwe_dir, subdir in TARGETS:
        label = f"{cwe_dir}/{subdir}" if subdir else cwe_dir
        print(f"  {label} … ", end="", flush=True)
        try:
            files, new = download_dir(cwe_dir, subdir)
            print(f"{len(files)} files, {new} new")
            total_new += new
        except Exception as e:
            print(f"ERROR: {e}")
    print(f"  → {total_new} new files downloaded\n")

    print("=== Scanning for primary test cases ===")
    entries = scan_testcases()
    print(f"  → {len(entries)} primary test cases found\n")

    print("=== Generating test_runner.c ===")
    runner_path = os.path.join(PROJ_ROOT, "stm32", "Core", "Src", "test_runner.c")
    with open(runner_path, "w") as f:
        f.write(generate_test_runner(entries))
    print(f"  → {runner_path}")

    print("=== Generating test_cases_testcases.h ===")
    header_path = os.path.join(PROJ_ROOT, "test_cases_testcases.h")
    with open(header_path, "w") as f:
        f.write(generate_header(entries))
    print(f"  → {header_path}")

    print(f"\nDone. {len(entries)} test cases in table.")

if __name__ == "__main__":
    main()
