#!/usr/bin/env python3
"""
Scan the juliet-dynamic submodule and auto-generate:
  - stm32/Core/Src/test_runner.c
  - test_cases_testcases.h

Only includes C (not C++) test families that do not require network/sockets.
Run from the project root after `git submodule update --init`.

Usage:
    python3 generate_test_runner.py
"""

import os
import re

PROJ_ROOT    = os.path.dirname(os.path.abspath(__file__))
JULIET_DIR   = os.path.join(PROJ_ROOT, "juliet-dynamic", "testcases")
LOCAL_TC_DIR = os.path.join(PROJ_ROOT, "stm32", "Core", "Src", "testcases")
RUNNER_OUT   = os.path.join(PROJ_ROOT, "stm32", "Core", "Src", "test_runner.c")
HEADER_OUT   = os.path.join(PROJ_ROOT, "test_cases_testcases.h")

# (base_dir, CWE-dir, subdir-or-None) — skip socket/network and C++-only dirs
TARGETS = [
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s02"),
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s03"),
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s04"),
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s05"),
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s06"),
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s07"),
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s08"),
    (JULIET_DIR,   "CWE121_Stack_Based_Buffer_Overflow", "s09"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s01"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s05"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s06"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s07"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s08"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s09"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s10"),
    (JULIET_DIR,   "CWE122_Heap_Based_Buffer_Overflow",  "s11"),
    (JULIET_DIR,   "CWE124_Buffer_Underwrite",           "s02"),
    (JULIET_DIR,   "CWE124_Buffer_Underwrite",           "s03"),
    (JULIET_DIR,   "CWE124_Buffer_Underwrite",           "s04"),
    (JULIET_DIR,   "CWE126_Buffer_Overread",             "s02"),
    # CWE415/416/476 are not in ispras/juliet-dynamic — kept as local files
    (LOCAL_TC_DIR, "CWE415_Double_Free",                 "s01"),
    (LOCAL_TC_DIR, "CWE416_Use_After_Free",              None),
    (LOCAL_TC_DIR, "CWE476_NULL_Pointer_Dereference",    None),
]

# Matches the top-level bad() entry point: void CWE..._01_bad() with no args
BAD_RE = re.compile(
    r'^void\s+(CWE\w+?_\d+[a-z]?_bad)\(\s*\)',
    re.MULTILINE
)

def find_primary(filepath):
    """Return (bad_fn, good_fn) if file defines a top-level _bad() entry."""
    with open(filepath, encoding="utf-8", errors="replace") as f:
        src = f.read()
    m = BAD_RE.search(src)
    if not m:
        return None
    bad_fn  = m.group(1)
    good_fn = bad_fn[:-4] + "_good"
    if f"void {good_fn}" not in src:
        return None
    return bad_fn, good_fn

def scan():
    entries = []
    for base_dir, cwe_dir, subdir in TARGETS:
        d = os.path.join(base_dir, cwe_dir, subdir) if subdir else os.path.join(base_dir, cwe_dir)
        if not os.path.isdir(d):
            print(f"  [skip] {d} — not found (submodule initialised?)")
            continue
        for fname in sorted(os.listdir(d)):
            if not fname.endswith(".c"):
                continue
            result = find_primary(os.path.join(d, fname))
            if result is None:
                continue
            bad_fn, good_fn = result
            cwe_num = re.match(r'(CWE\d+)', fname).group(1)
            suffix  = fname[:-2].split("__", 1)[1] if "__" in fname else fname[:-2]
            label   = f"{cwe_num} ({suffix})"
            entries.append((label, bad_fn, good_fn))
    return entries

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

#define MPU_RNR                (*(volatile uint32_t *)0xE000ED98)
#define MPU_RLAR               (*(volatile uint32_t *)0xE000EDA0)
#define MPU_PRIVILEGED_DEFAULT 4U

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

def generate_runner(entries):
    fwd   = "\n".join(f"void {b}(void);\nvoid {g}(void);" for _, b, g in entries)
    table = "\n".join(
        f'    {{ "{lbl}",\n      {b},\n      {g} }},'
        for lbl, b, g in entries
    )
    return RUNNER_TEMPLATE.format(forward_decls=fwd, table_entries=table)

def generate_header(entries):
    lines = ["#ifndef TEST_CASES_H", "#define TEST_CASES_H", ""]
    prev  = None
    for lbl, bad_fn, good_fn in entries:
        cwe = re.match(r'(CWE\d+)', lbl).group(1)
        if cwe != prev:
            lines.append(f"\n/* ── {cwe} ── */")
            prev = cwe
        lines += [f"void {bad_fn}(void);", f"void {good_fn}(void);"]
    lines += ["", "#endif"]
    return "\n".join(lines) + "\n"

def main():
    if not os.path.isdir(JULIET_DIR):
        print("ERROR: juliet-dynamic submodule not found.")
        print("Run: git submodule update --init")
        return

    print("Scanning test cases …")
    entries = scan()
    print(f"  {len(entries)} primary test cases found")

    with open(RUNNER_OUT, "w") as f:
        f.write(generate_runner(entries))
    print(f"  wrote {RUNNER_OUT}")

    with open(HEADER_OUT, "w") as f:
        f.write(generate_header(entries))
    print(f"  wrote {HEADER_OUT}")

if __name__ == "__main__":
    main()
