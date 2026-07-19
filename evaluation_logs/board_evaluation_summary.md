# STM32 Juliet Dynamic CWE Board Evaluation

Date: 2026-07-18

## Scope

- Juliet dynamic source tree: `juliet-dynamic/testcases`
- Built firmware pairs: 36 pass/nopass pairs, 72 ELF images
- Board runs attempted: 112, including 10 CWE191 reruns, 10 UART-recovery flashes, 3 CWE416 UAF-instrumentation runs, 10 library-wrapper validation runs, 2 CWE476 NULL-check runs, and 5 CWE121 fault-diagnostic runs
- Completed UART captures with `Result:`: 104 total
- Complete evaluation groups: 72/72; six previously interrupted groups were reconstructed from their original prefix and completed recovery suffix logs
- Covered CWE categories: CWE121, CWE122, CWE124, CWE126, CWE127, CWE191, CWE415, CWE416, CWE476
- Not covered by current build filter: CWE190 and I/O-dependent Juliet cases filtered by `fgets`, `socket`, `fscanf`, `file_`
- Detection granularity filter: exclude memory overflows and underflows smaller than 8 bytes
- Excluded testcase families: `CWE129` (single 4-byte `int` out-of-bounds access), `CWE193` (1-byte `char` or 4-byte `wchar_t` terminator overflow), and the three ARM `sizeof(pointer)` cases in CWE122 s11 (`double`, `int64_t`, and `struct`, each a 4-byte overflow)
- Excluded observed results: 73; accesses at exactly 8 bytes and variable/unbounded violations remain included
- CWE191 was rebuilt and rerun after adding `integer-underflow-pass`; its 10 complete board logs are in `evaluation_logs/cwe191_underflow_pass`
- The 8-byte memory-boundary filter does not apply to CWE191 arithmetic errors

## Static Size

Static size covers each complete firmware image, including testcase code excluded
from the granularity-filtered detection results below. This table preserves the
pre-UART-recovery identical-runner comparison; the current CWE416 image sizes
are reported separately in the re-evaluation section.

| CWE | Pairs | Avg flash pass | Avg flash nopass | Flash overhead | Avg RAM pass | Avg RAM nopass | RAM diff |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ALL | 36 | 227053 B | 218962 B | 3.70% | 36968 B | 36976 B | -8 B |
| CWE121 | 9 | 228948 B | 219570 B | 4.27% | 36968 B | 36976 B | -8 B |
| CWE122 | 8 | 226346 B | 218880 B | 3.41% | 36968 B | 36976 B | -8 B |
| CWE124 | 4 | 226840 B | 218694 B | 3.72% | 36968 B | 36976 B | -8 B |
| CWE126 | 3 | 228043 B | 219085 B | 4.09% | 36968 B | 36976 B | -8 B |
| CWE127 | 4 | 226994 B | 218710 B | 3.79% | 36968 B | 36976 B | -8 B |
| CWE191 | 5 | 225754 B | 218725 B | 3.21% | 36968 B | 36976 B | -8 B |
| CWE415 | 1 | 224032 B | 218032 B | 2.75% | 36968 B | 36976 B | -8 B |
| CWE416 | 1 | 225952 B | 218784 B | 3.28% | 36968 B | 36976 B | -8 B |
| CWE476 | 1 | 224360 B | 218160 B | 2.84% | 36968 B | 36976 B | -8 B |

## Board Results

Results below apply the 8-byte granularity filter. `ResultPASS` is recomputed
from retained unique testcases. For the six recovered groups, duplicate
prefix/suffix observations are deduplicated by testcase name.

| CWE | Mode | Complete logs | ResultPASS | PASS | MISS | FP | BOTH | Excluded |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CWE121 | pass | 9/9 | 88/90 | 88 | 2 | 0 | 0 | 22 |
| CWE121 | nopass | 9/9 | 41/90 | 41 | 49 | 0 | 0 | 22 |
| CWE122 | pass | 8/8 | 46/49 | 46 | 3 | 0 | 0 | 15 |
| CWE122 | nopass | 8/8 | 30/49 | 30 | 18 | 1 | 0 | 10 |
| CWE124 | pass | 4/4 | 31/32 | 31 | 1 | 0 | 0 | 0 |
| CWE124 | nopass | 4/4 | 18/32 | 18 | 14 | 0 | 0 | 0 |
| CWE126 | pass | 3/3 | 18/24 | 18 | 6 | 0 | 0 | 2 |
| CWE126 | nopass | 3/3 | 4/24 | 4 | 20 | 0 | 0 | 2 |
| CWE127 | pass | 4/4 | 32/32 | 32 | 0 | 0 | 0 | 0 |
| CWE127 | nopass | 4/4 | 5/32 | 5 | 27 | 0 | 0 | 0 |
| CWE191 | pass | 5/5 | 16/38 | 16 | 10 | 8 | 4 | 0 |
| CWE191 | nopass | 5/5 | 0/38 | 0 | 38 | 0 | 0 | 0 |
| CWE415 | pass | 1/1 | 6/6 | 6 | 0 | 0 | 0 | 0 |
| CWE415 | nopass | 1/1 | 6/6 | 6 | 0 | 0 | 0 | 0 |
| CWE416 | pass | 1/1 | 7/7 | 7 | 0 | 0 | 0 | 0 |
| CWE416 | nopass | 1/1 | 0/7 | 0 | 7 | 0 | 0 | 0 |
| CWE476 | pass | 1/1 | 8/9 | 8 | 1 | 0 | 0 | 0 |
| CWE476 | nopass | 1/1 | 7/9 | 7 | 2 | 0 | 0 | 0 |

## UART Recovery

- All six `capture-idle-timeout` groups now have complete testcase observations.
- `capture_uart.py` automatically reopens the serial device for up to 15 seconds when ST-Link VCP disconnects during flash/reset.
- Nopass stack overflows were corrupting the test runner's stack-local testcase pointer and causing a post-test BusFault. Runner bookkeeping now uses static storage.
- `generate_test_runner.py` supports 1-based `--start-index` and `--end-index`; `build_per_cwe.sh` exposes these as `TEST_START_INDEX` and `TEST_END_INDEX` for suffix recovery.
- Recovery logs are retained as `evaluation_logs/recovery_*.log`. Full-range firmware images were rebuilt after recovery.

## Regressions

- CWE122_Heap_Based_Buffer_Overflow_s06 nopass: `FAIL-FP` in `CWE135_01`
- CWE191 pass: 8 `FAIL-FP` and 4 `FAIL-BOTH`, all in `rand` testcase families; fixed-minimum testcases do not produce good-path false positives

## Library Wrapper Re-evaluation

- Added pass-side wrappers for `snprintf`, `swprintf`, `wcsncat`, `calloc`,
  and `realloc`.
- `wcsncat` initially assumed that all `n` characters were written and produced
  two good-path false positives in
  `CWE121_Stack_Based_Buffer_Overflow_s05_pass_wrappers_v2.log`. The accepted
  implementation checks `min(n, wcslen(src))` plus the terminator and the final
  `CWE121 s05` run reached 15/15 PASS.
- Accepted pass validation logs:
  `CWE121_Stack_Based_Buffer_Overflow_s03_pass_wrappers_v1.log`,
  `CWE121_Stack_Based_Buffer_Overflow_s05_pass_wrappers_v3.log`,
  `CWE121_Stack_Based_Buffer_Overflow_s06_pass_wrappers_v1.log`,
  `CWE121_Stack_Based_Buffer_Overflow_s07_pass_wrappers_v1.log`,
  `CWE121_Stack_Based_Buffer_Overflow_s08_pass_wrappers_v1.log`,
  `CWE122_Heap_Based_Buffer_Overflow_s06_pass_wrappers_v1.log`,
  `CWE122_Heap_Based_Buffer_Overflow_s08_pass_wrappers_v1.log`,
  `CWE122_Heap_Based_Buffer_Overflow_s09_pass_wrappers_v1.log`, and
  `CWE122_Heap_Based_Buffer_Overflow_s10_pass_wrappers_v1.log`.
- This raises the non-rand, >=8-byte aggregate from 238/266 to 248/266
  before the later NULL/fault fixes. CWE121 improves from 81/90 to 87/90, and CWE122 improves from
  42/49 to 46/49.
- Three `wchar_t swprintf` cases remain missed because the checked endpoint can
  stay inside the same stack/heap tag region. Treat these with the same
  subobject-boundary limitation as field-level heap overflows rather than as a
  missing libc wrapper.

## NULL and Fault-Diagnostic Re-evaluation

- Added explicit pass-side `check_null_ptr()` instrumentation for Juliet CWE
  functions before GEP base use and load/store pointer operands.
- `CWE476_NULL_Pointer_Dereference_pass_nullcheck_v1.log` improves pass from
  7/9 to 8/9 by detecting `binary_if_01`; nopass remains 7/9 in
  `CWE476_NULL_Pointer_Dereference_nopass_nullcheck_v1.log`.
- `null_check_after_deref_01` remains a miss because the board run's
  `malloc(sizeof(int))` succeeds, so the bad path does not actually dereference
  NULL under the current allocator behavior.
- Added fault records with path phase, stacked PC, LR, CFSR, HFSR, MMFAR, and
  BFAR. The intermediate diagnostic logs are
  `CWE121_Stack_Based_Buffer_Overflow_s01_pass_faultdiag_v1.log` through
  `v4.log`.
- The accepted `CWE121_Stack_Based_Buffer_Overflow_s01_pass_faultdiag_v5.log`
  fixes the `CWE135_01` good-path false positive. Root cause: stack MPU redzone
  state was stale after dynamic `alloca`, and runtime string wrappers could also
  create callee-frame redzones overlapping caller stack buffers. Runtime
  wrappers are now excluded from stack MPU instrumentation, and dynamic `alloca`
  refreshes the redzone with scratch stack space around the reconfiguration
  call.
- Current non-rand, >=8-byte aggregate after all accepted fixes is 250/266
  (94.0%) with zero good-path false positives.

## CWE416 UAF Re-evaluation

- `check_live_tag()` now reports a fault only when the addressed 8-byte shadow
  granule contains `UNPOISON_TAG`.
- `arithmetic-pointer-tag-pass` inserts this check for pointer arguments of the
  known dereference sinks `printLine`, `printWLine`, and `printStructLine`.
- The final pass run improved from 3/7 to 7/7, while nopass remained 0/7. No
  good-path false positive occurred in the accepted run.
- An intermediate experiment that instrumented every load/store produced 2/7
  PASS and five good-path false positives. It was rejected because its inserted
  calls changed the behavior of the existing MPU call-frame instrumentation.
- Final logs:
  `evaluation_logs/CWE416_Use_After_Free_pass_uaf_v3.log` and
  `evaluation_logs/CWE416_Use_After_Free_nopass_uaf_v3.log`.
- The final CWE416 images use 226224 B versus 218896 B Flash, a 7328 B
  (3.35%) difference. Static RAM is 37184 B versus 37192 B. These current-runner
  values are kept separate from the pre-recovery 72-image static-size table
  above.
- Mean bad-path time is 10348.9 us for pass versus 94.6 us for nopass. This
  includes tag-mismatch reporting and `longjmp` recovery and is not pure
  instrumentation overhead.

## Assessment

After excluding fixed violations smaller than the detector's 8-byte
granularity, the project is partially valid as a memory-error detector on this
board. It is strong for buffer underwrite/underread and double free, and the
library-wrapper recovery substantially improves stack/heap buffer overflows.
Use-after-free reaches 7/7 for the
evaluated baseline sink set after adding freed-tag sink checks, although this
does not establish complete temporal safety for unmodeled sinks or address
reuse.
The new integer-underflow pass improves CWE191 from 0/38 to 16/38 (42.1%),
while nopass remains 0/38. This is a real detection gain for fixed-minimum
integer operations, but the 8 false positives and 4 both-path failures in
random-input cases prevent treating CWE191 support as reliable yet. NULL
dereference results are not a strong validation signal because pass and
nopass both report 7/9 PASS.

Dynamic timing is dominated by fault detection and recovery paths, so it
should not be interpreted as pure instrumentation overhead. For the new
CWE191 run, pass averages 844.4 us on bad paths and 116.1 us on good paths,
versus 63.3 us and 67.4 us for nopass. Static overhead remains modest: average
Flash increases by 3.70%, while static RAM is effectively unchanged.
