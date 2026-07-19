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

import argparse
import os
import re

PROJ_ROOT    = os.path.dirname(os.path.abspath(__file__))
JULIET_DIR   = os.path.join(PROJ_ROOT, "juliet-dynamic", "testcases")
LOCAL_TC_DIR = os.path.join(PROJ_ROOT, "stm32", "Core", "Src", "testcases")
RUNNER_OUT   = os.path.join(PROJ_ROOT, "stm32", "Core", "Src", "test_runner.c")
HEADER_OUT   = os.path.join(PROJ_ROOT, "test_cases_testcases.h")

# Subdirs whose first .c file name contains these patterns use stdin/sockets
# and are incompatible with bare-metal STM32 — skip them.
SKIP_SOURCE_PATTERNS = ["socket", "fscanf", "fgets", "file_"]
BENCHMARK_EXCLUSION_FILE = os.path.join(PROJ_ROOT, "evaluation_logs", "evaluation_exclusions.txt")


def _has_c_files(dirpath):
    return any(f.endswith(".c") for f in os.listdir(dirpath))


def _is_compatible_subdir(dirpath):
    """Return True if the dir has at least one compatible *_01.c primary file."""
    primary_files = sorted(f for f in os.listdir(dirpath) if f.endswith("_01.c"))
    if not primary_files:
        return False
    return any(
        not any(pat in f.lower() for pat in SKIP_SOURCE_PATTERNS)
        for f in primary_files
    )


def discover_targets():
    """Walk juliet-dynamic/testcases and return all compatible (base, cwe, subdir) tuples."""
    if not os.path.isdir(JULIET_DIR):
        print(f"  [warn] {JULIET_DIR} not found — falling back to LOCAL_TC_DIR")
        return []
    targets = []
    for cwe_dir in sorted(os.listdir(JULIET_DIR)):
        if not cwe_dir.startswith("CWE"):
            continue
        cwe_path = os.path.join(JULIET_DIR, cwe_dir)
        if not os.path.isdir(cwe_path):
            continue
        subdirs = sorted(
            d for d in os.listdir(cwe_path)
            if re.match(r"s\d+$", d) and os.path.isdir(os.path.join(cwe_path, d))
        )
        if subdirs:
            for sd in subdirs:
                if _is_compatible_subdir(os.path.join(cwe_path, sd)):
                    targets.append((JULIET_DIR, cwe_dir, sd))
        else:
            if _has_c_files(cwe_path) and _is_compatible_subdir(cwe_path):
                targets.append((JULIET_DIR, cwe_dir, None))
    return targets

# Matches the top-level bad() entry point: void CWE..._01_bad() with no args
BAD_RE = re.compile(
    r'^void\s+(CWE\w+?_\d+[a-z]?_bad)\(\s*\)',
    re.MULTILINE
)

def find_primary(filepath):
    """Return (bad_fn, good_fn) if file defines a top-level _bad() entry."""
    try:
        with open(filepath, encoding="utf-8", errors="replace") as f:
            src = f.read()
    except OSError:
        return None
    m = BAD_RE.search(src)
    if not m:
        return None
    bad_fn  = m.group(1)
    good_fn = bad_fn[:-4] + "_good"
    if f"void {good_fn}" not in src:
        return None
    return bad_fn, good_fn

def load_benchmark_exclusions():
    if os.environ.get("BENCHMARK_MODE") != "1":
        return []

    path = os.environ.get("BENCHMARK_EXCLUSIONS", BENCHMARK_EXCLUSION_FILE)
    patterns = []
    try:
        with open(path, encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    patterns.append(re.compile(line))
    except OSError:
        pass
    return patterns


def scan(cwe_filter=None, subdir_filter=None):
    entries = []
    benchmark_exclusions = load_benchmark_exclusions()
    targets = discover_targets()
    if cwe_filter:
        targets = [(b, c, s) for b, c, s in targets if c.startswith(cwe_filter)]
    if subdir_filter:
        targets = [(b, c, s) for b, c, s in targets if s == subdir_filter or s is None]
    if not targets:
        print("  [warn] no targets discovered from juliet-dynamic")
        return entries
    for base_dir, cwe_dir, subdir in targets:
        d = os.path.join(base_dir, cwe_dir, subdir) if subdir else os.path.join(base_dir, cwe_dir)
        if not os.path.isdir(d):
            continue
        for fname in sorted(os.listdir(d)):
            if not fname.endswith("_01.c"):
                continue
            if any(pat in fname.lower() for pat in SKIP_SOURCE_PATTERNS):
                continue
            result = find_primary(os.path.join(d, fname))
            if result is None:
                continue
            bad_fn, good_fn = result
            cwe_num = re.match(r'(CWE\d+)', fname).group(1)
            suffix  = fname[:-2].split("__", 1)[1] if "__" in fname else fname[:-2]
            label   = f"{cwe_num} ({suffix})"
            if any(pattern.search(label) for pattern in benchmark_exclusions):
                continue
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
volatile int g_test_phase     = 0;
volatile DetectionSource g_detection_source = DETECTION_NONE;
volatile uint32_t g_detection_cycles = 0;
volatile uint32_t g_detection_start_cycle = 0;
volatile uint32_t g_last_fault_phase = 0;
volatile uint32_t g_last_fault_pc    = 0;
volatile uint32_t g_last_fault_lr    = 0;
volatile uint32_t g_last_fault_cfsr  = 0;
volatile uint32_t g_last_fault_hfsr  = 0;
volatile uint32_t g_last_fault_mmfar = 0;
volatile uint32_t g_last_fault_bfar  = 0;

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

static const char *detection_source_name(DetectionSource source) {{
    switch (source) {{
    case DETECTION_SOFTWARE_TAG: return "software-tag";
    case DETECTION_NULL: return "null";
    case DETECTION_INTEGER: return "integer";
    case DETECTION_MPU: return "mpu";
    case DETECTION_CPU_FAULT: return "cpu-fault";
    default: return "none";
    }}
}}

/* ── stack canary ────────────────────────────────────────────────────── */
#define STACK_CANARY_SIZE   1536U
#define STACK_CANARY_FILL   0xCCU
#define STACK_CANARY_MARGIN  128U

static uint8_t *s_canary_base = NULL;

__attribute__((noinline))
static void stack_canary_arm(void) {{
    register uint32_t sp __asm("sp");
    s_canary_base = (uint8_t *)(sp - STACK_CANARY_MARGIN - STACK_CANARY_SIZE);
    for (uint32_t i = 0; i < STACK_CANARY_SIZE; i++)
        s_canary_base[i] = STACK_CANARY_FILL;
}}

static uint32_t stack_canary_measure(void) {{
    if (!s_canary_base) return 0;
    for (uint32_t i = 0; i < STACK_CANARY_SIZE; i++) {{
        if (s_canary_base[i] != STACK_CANARY_FILL)
            return STACK_CANARY_SIZE - i;
    }}
    return 0;
}}

/* ── state reset between test cases ─────────────────────────────────── */
extern void HAL_MPU_Disable(void);
extern void HAL_MPU_Enable(uint32_t MPU_Control);
extern void configure_mpu_for_null_ptr(void);
extern void heap_reset(void);
extern void tags_reset(void);
extern size_t heap_get_peak(void);

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

/*
 * Keep runner bookkeeping out of the runner stack. Nopass stack-overflow
 * tests can legally overwrite their caller's frame before returning; storing
 * the current table pointer and verdict data here lets the runner still emit
 * a result for that test.
 */
static const TestCase *s_tc;
static char s_result_buf[160];
static int s_test_index;
static int s_pass;
static int s_fail;
static uint32_t s_t0;
static uint32_t s_bad_us, s_good_us;
static uint32_t s_bad_heap, s_good_heap;
static uint32_t s_bad_stk, s_good_stk;
static int s_bad_caught, s_good_fp;
static DetectionSource s_bad_source, s_good_source;
static uint32_t s_bad_detection_cycles, s_good_detection_cycles;
static const char *s_verdict;

static void clear_fault_record(void) {{
    g_detection_source = DETECTION_NONE;
    g_detection_cycles = 0;
    g_detection_start_cycle = 0;
    g_last_fault_phase = 0;
    g_last_fault_pc = 0;
    g_last_fault_lr = 0;
    g_last_fault_cfsr = 0;
    g_last_fault_hfsr = 0;
    g_last_fault_mmfar = 0;
    g_last_fault_bfar = 0;
}}

static const char *fault_phase_name(uint32_t phase) {{
    if (phase == 1U) return "bad";
    if (phase == 2U) return "good";
    return "idle";
}}

static void emit_fault_record(const char *path) {{
    if ((g_last_fault_pc | g_last_fault_cfsr | g_last_fault_hfsr |
         g_last_fault_mmfar | g_last_fault_bfar) == 0U) {{
        return;
    }}

    snprintf(s_result_buf, sizeof(s_result_buf),
             "[FAULT] path=%s phase=%s pc=0x%08lX lr=0x%08lX cfsr=0x%08lX hfsr=0x%08lX mmfar=0x%08lX bfar=0x%08lX\\r\\n",
             path, fault_phase_name((uint32_t)g_last_fault_phase),
             (unsigned long)g_last_fault_pc, (unsigned long)g_last_fault_lr,
             (unsigned long)g_last_fault_cfsr, (unsigned long)g_last_fault_hfsr,
             (unsigned long)g_last_fault_mmfar, (unsigned long)g_last_fault_bfar);
    uart_send_string(s_result_buf);
}}

/* ── runner ──────────────────────────────────────────────────────────── */
#ifndef BENCHMARK_MODE
void run_all_tests(void) {{
    dwt_init();

    s_pass = 0;
    s_fail = 0;

    uart_send_string("\\r\\n========================================\\r\\n");
    uart_send_string("  Memory Error Detection Test Runner\\r\\n");
    uart_send_string("========================================\\r\\n");

    for (s_test_index = 0; s_test_index < (int)NUM_TESTS; s_test_index++) {{
        s_tc = &test_table[s_test_index];

        /* ---- bad path: must detect ---- */
        reset_test_state();
        clear_fault_record();
        g_error_detected = 0;
        g_test_running   = 1;
        g_test_phase     = 1;
        stack_canary_arm();
        if (setjmp(g_test_recovery) == 0) {{
            s_t0 = DWT_CYCCNT;
            g_detection_start_cycle = s_t0;
            s_tc->bad();
        }}
        reset_mpu();
        s_bad_us = dwt_elapsed_us(s_t0);
        s_bad_heap = (uint32_t)heap_get_peak();
        s_bad_stk = stack_canary_measure();
        s_bad_source = g_detection_source;
        s_bad_detection_cycles = g_detection_cycles;
        s_bad_caught = g_error_detected &&
                       s_bad_source != DETECTION_CPU_FAULT;
        emit_fault_record("bad");
        g_test_phase = 0;
        g_test_running = 0;

        /* ---- good path: must NOT detect ---- */
        reset_test_state();
        clear_fault_record();
        g_error_detected = 0;
        g_test_running   = 1;
        g_test_phase     = 2;
        stack_canary_arm();
        if (setjmp(g_test_recovery) == 0) {{
            s_t0 = DWT_CYCCNT;
            g_detection_start_cycle = s_t0;
            s_tc->good();
        }}
        reset_mpu();
        s_good_us = dwt_elapsed_us(s_t0);
        s_good_heap = (uint32_t)heap_get_peak();
        s_good_stk = stack_canary_measure();
        s_good_fp = g_error_detected;
        s_good_source = g_detection_source;
        s_good_detection_cycles = g_detection_cycles;
        emit_fault_record("good");
        g_test_phase = 0;
        g_test_running = 0;

        if      ( s_bad_caught && !s_good_fp) {{ s_verdict = "PASS     "; s_pass++; }}
        else if (!s_bad_caught && !s_good_fp) {{ s_verdict = "FAIL-MISS"; s_fail++; }}
        else if ( s_bad_caught &&  s_good_fp) {{ s_verdict = "FAIL-FP  "; s_fail++; }}
        else                                  {{ s_verdict = "FAIL-BOTH"; s_fail++; }}

        snprintf(s_result_buf, sizeof(s_result_buf),
                 "[%s] %s\\r\\n"
                 "  bad:  %6lu us  heap:%5luB  stk:%5luB\\r\\n"
                 "  good: %6lu us  heap:%5luB  stk:%5luB\\r\\n",
                 s_verdict, s_tc->name,
                 (unsigned long)s_bad_us,  (unsigned long)s_bad_heap,  (unsigned long)s_bad_stk,
                 (unsigned long)s_good_us, (unsigned long)s_good_heap, (unsigned long)s_good_stk);
        uart_send_string(s_result_buf);

        snprintf(s_result_buf, sizeof(s_result_buf),
                 "[DETECT] bad_source=%s good_source=%s bad_cycles=%lu good_cycles=%lu\\r\\n",
                 detection_source_name(s_bad_source),
                 detection_source_name(s_good_source),
                 (unsigned long)s_bad_detection_cycles,
                 (unsigned long)s_good_detection_cycles);
        uart_send_string(s_result_buf);

        snprintf(s_result_buf, sizeof(s_result_buf),
                 "[METRIC] bad_us=%lu good_us=%lu bad_heap=%lu good_heap=%lu bad_stk=%lu good_stk=%lu\\r\\n",
                 (unsigned long)s_bad_us,  (unsigned long)s_good_us,
                 (unsigned long)s_bad_heap, (unsigned long)s_good_heap,
                 (unsigned long)s_bad_stk,   (unsigned long)s_good_stk);
        uart_send_string(s_result_buf);
    }}

    uart_send_string("----------------------------------------\\r\\n");
    snprintf(s_result_buf, sizeof(s_result_buf),
             "  Result: %d / %d PASS\\r\\n", s_pass, (int)NUM_TESTS);
    uart_send_string(s_result_buf);
    uart_send_string("========================================\\r\\n");
}}
#else

#ifndef BENCHMARK_WARMUP
#define BENCHMARK_WARMUP 20U
#endif
#ifndef BENCHMARK_SAMPLES
#define BENCHMARK_SAMPLES 200U
#endif

static uint32_t s_bench_samples[BENCHMARK_SAMPLES];
static char s_bench_result_buf[320];

static void sort_samples(uint32_t *samples, uint32_t count) {{
    for (uint32_t i = 1; i < count; ++i) {{
        uint32_t value = samples[i];
        uint32_t j = i;
        while (j > 0 && samples[j - 1] > value) {{
            samples[j] = samples[j - 1];
            --j;
        }}
        samples[j] = value;
    }}
}}

static int run_good_sample(uint32_t *cycles) {{
    reset_test_state();
    clear_fault_record();
    g_error_detected = 0;
    g_test_running = 1;
    g_test_phase = 2;

    int completed = 0;
    if (setjmp(g_test_recovery) == 0) {{
        s_t0 = DWT_CYCCNT;
        g_detection_start_cycle = s_t0;
        s_tc->good();
        *cycles = DWT_CYCCNT - s_t0;
        completed = 1;
    }}

    g_test_phase = 0;
    g_test_running = 0;
    return completed && !g_error_detected;
}}

void run_all_tests(void) {{
    dwt_init();
    uart_send_string("\\r\\n========================================\\r\\n");
    uart_send_string("  Memory Error Detector Microbenchmark\\r\\n");
    uart_send_string("========================================\\r\\n");

    for (s_test_index = 0; s_test_index < (int)NUM_TESTS; ++s_test_index) {{
        s_tc = &test_table[s_test_index];
        uint32_t ignored_cycles = 0;
        uint32_t valid = 0;
        uint32_t faults = 0;
        uint64_t sum = 0;
        uint64_t sumsq = 0;

        for (uint32_t i = 0; i < BENCHMARK_WARMUP; ++i) {{
            (void)run_good_sample(&ignored_cycles);
        }}

        for (uint32_t i = 0; i < BENCHMARK_SAMPLES; ++i) {{
            uint32_t cycles = 0;
            if (run_good_sample(&cycles)) {{
                s_bench_samples[valid++] = cycles;
                sum += cycles;
                sumsq += (uint64_t)cycles * cycles;
            }} else {{
                ++faults;
            }}
        }}

        reset_test_state();
        clear_fault_record();
        g_error_detected = 0;
        g_test_running = 1;
        g_test_phase = 2;
        stack_canary_arm();
        if (setjmp(g_test_recovery) == 0) {{
            s_tc->good();
        }}
        s_good_heap = (uint32_t)heap_get_peak();
        s_good_stk = stack_canary_measure();
        s_good_source = g_detection_source;
        g_test_phase = 0;
        g_test_running = 0;

        if (valid > 0) {{
            sort_samples(s_bench_samples, valid);
            uint32_t p95_index = ((valid * 95U + 99U) / 100U) - 1U;
            snprintf(
                s_bench_result_buf, sizeof(s_bench_result_buf),
                "[BENCH] name=\\"%s\\" n=%lu warmup=%u min_cycles=%lu median_cycles=%lu p95_cycles=%lu max_cycles=%lu mean_cycles=%lu sum_cycles=%llu sumsq_cycles=%llu good_heap=%lu good_stk=%lu faults=%lu source=%s\\r\\n",
                s_tc->name, (unsigned long)valid, (unsigned)BENCHMARK_WARMUP,
                (unsigned long)s_bench_samples[0],
                (unsigned long)s_bench_samples[valid / 2U],
                (unsigned long)s_bench_samples[p95_index],
                (unsigned long)s_bench_samples[valid - 1U],
                (unsigned long)(sum / valid),
                (unsigned long long)sum,
                (unsigned long long)sumsq,
                (unsigned long)s_good_heap,
                (unsigned long)s_good_stk,
                (unsigned long)faults,
                detection_source_name(s_good_source));
        }} else {{
            snprintf(
                s_bench_result_buf, sizeof(s_bench_result_buf),
                "[BENCH] name=\\"%s\\" n=0 warmup=%u faults=%lu source=%s\\r\\n",
                s_tc->name, (unsigned)BENCHMARK_WARMUP,
                (unsigned long)faults,
                detection_source_name(s_good_source));
        }}
        uart_send_string(s_bench_result_buf);
    }}

    snprintf(s_bench_result_buf, sizeof(s_bench_result_buf),
             "  Result: %d / %d BENCH\\r\\n", (int)NUM_TESTS, (int)NUM_TESTS);
    uart_send_string(s_bench_result_buf);
    uart_send_string("========================================\\r\\n");
}}
#endif
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
    parser = argparse.ArgumentParser()
    parser.add_argument("--cwe", default=None,
                        help="Filter to one CWE prefix (e.g. CWE121)")
    parser.add_argument("--subdir", default=None,
                        help="Filter to one subdir (e.g. s02)")
    parser.add_argument("--start-index", type=int, default=1,
                        help="First testcase to include (1-based, default: 1)")
    parser.add_argument("--end-index", type=int, default=None,
                        help="Last testcase to include (1-based, inclusive)")
    args = parser.parse_args()

    print("Scanning test cases …")
    entries = scan(cwe_filter=args.cwe, subdir_filter=args.subdir)
    total = len(entries)
    if args.start_index < 1:
        parser.error("--start-index must be at least 1")
    if args.end_index is not None and args.end_index < args.start_index:
        parser.error("--end-index must be greater than or equal to --start-index")

    start = args.start_index - 1
    end = args.end_index
    entries = entries[start:end]
    print(f"  selected {len(entries)} of {total} primary test cases"
          f" (index {args.start_index}.."
          f"{args.end_index if args.end_index is not None else total})")

    with open(RUNNER_OUT, "w") as f:
        f.write(generate_runner(entries))
    print(f"  wrote {RUNNER_OUT}")

    with open(HEADER_OUT, "w") as f:
        f.write(generate_header(entries))
    print(f"  wrote {HEADER_OUT}")

if __name__ == "__main__":
    main()
