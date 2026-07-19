#!/usr/bin/env bash
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HELPER="/home/jeon/.codex/skills/stm32-cwe-board-test/scripts/cwe_board_test.py"
UART="${UART:-/dev/ttyACM0}"
TIMEOUT="${TIMEOUT:-180}"
IDLE_TIMEOUT="${IDLE_TIMEOUT:-45}"
FLASH_TIMEOUT="${FLASH_TIMEOUT:-60}"
LOG_DIR="${LOG_DIR:-$ROOT/evaluation_logs/microbench_remainders}"
FAILURES="$LOG_DIR/failures.tsv"

mkdir -p "$LOG_DIR"
: > "$FAILURES"

flash_elf() {
    local elf="$1"
    timeout -k 5s "${FLASH_TIMEOUT}s" python3 - "$HELPER" "$elf" <<'PY'
import importlib.util
import sys

helper, elf = sys.argv[1], sys.argv[2]
spec = importlib.util.spec_from_file_location("cwe_board_test", helper)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
command = "program {" + elf + "} verify reset"
print(module.tcl_command("127.0.0.1", 6666, command))
PY
}

build_one() {
    local cwe="$1"
    local subdir="$2"
    local mode="$3"
    local index="$4"
    local subdir_args=()

    if [ "$subdir" != "-" ]; then
        subdir_args=("$subdir")
    fi

    case "$mode" in
        full)
            TEST_START_INDEX="$index" TEST_END_INDEX="$index" \
                BENCHMARK_MODE=1 "$ROOT/build_per_cwe.sh" "$cwe" "${subdir_args[@]}"
            ;;
        runtime)
            TEST_START_INDEX="$index" TEST_END_INDEX="$index" \
                BENCHMARK_MODE=1 NO_PASS=1 "$ROOT/build_per_cwe.sh" "$cwe" "${subdir_args[@]}"
            ;;
        native)
            TEST_START_INDEX="$index" TEST_END_INDEX="$index" \
                BENCHMARK_MODE=1 NATIVE_BASELINE=1 "$ROOT/build_per_cwe.sh" "$cwe" "${subdir_args[@]}"
            ;;
        *)
            echo "unknown mode: $mode" >&2
            return 2
            ;;
    esac
}

run_one() {
    local current="$1"
    local total="$2"
    local cwe="$3"
    local subdir="$4"
    local mode="$5"
    local index="$6"
    local label="$cwe"
    local suffix

    if [ "$subdir" != "-" ]; then
        label="${label}_${subdir}"
    fi

    case "$mode" in
        full) suffix="_bench" ;;
        runtime) suffix="_bench_nopass" ;;
        native) suffix="_bench_native" ;;
        *) echo "unknown mode: $mode" >&2; return 2 ;;
    esac

    local log="$LOG_DIR/${label}_${mode}_idx${index}.log"
    local elf="$ROOT/firmware/firmware_${label}${suffix}.elf"

    if grep -q 'Result:.*BENCH' "$log" 2>/dev/null; then
        echo "[$current/$total] SKIP $label $mode idx=$index"
        return 0
    fi

    if ! python3 "$HELPER" preflight --uart "$UART" >/dev/null; then
        printf '%s\t%s\t%s\tboard-disconnected\n' "$label" "$mode" "$index" >> "$FAILURES"
        return 2
    fi

    echo "[$current/$total] BUILD $label $mode idx=$index"
    if ! build_one "$cwe" "$subdir" "$mode" "$index"; then
        printf '%s\t%s\t%s\tbuild\n' "$label" "$mode" "$index" >> "$FAILURES"
        return 1
    fi

    if [ ! -f "$elf" ]; then
        printf '%s\t%s\t%s\tmissing-elf\n' "$label" "$mode" "$index" >> "$FAILURES"
        return 1
    fi

    echo "[$current/$total] RUN   $label $mode idx=$index"
    timeout -k 5s "${TIMEOUT}s" \
        python3 "$ROOT/capture_uart.py" "$UART" "$log" --baud 115200 &
    local capture_pid=$!
    sleep 1

    if ! kill -0 "$capture_pid" 2>/dev/null; then
        wait "$capture_pid" 2>/dev/null || true
        printf '%s\t%s\t%s\tcapture-open\n' "$label" "$mode" "$index" >> "$FAILURES"
        return 2
    fi

    if ! flash_elf "$elf"; then
        kill "$capture_pid" 2>/dev/null || true
        wait "$capture_pid" 2>/dev/null || true
        printf '%s\t%s\t%s\tflash\n' "$label" "$mode" "$index" >> "$FAILURES"
        return 1
    fi

    local start now last_write
    start="$(date +%s)"
    last_write="$start"
    while kill -0 "$capture_pid" 2>/dev/null; do
        if grep -q 'Result:.*BENCH' "$log" 2>/dev/null; then
            kill "$capture_pid" 2>/dev/null || true
            wait "$capture_pid" 2>/dev/null || true
            return 0
        fi

        now="$(date +%s)"
        if [ -s "$log" ]; then
            last_write="$(stat -c %Y "$log")"
        fi

        if [ $((now - start)) -ge "$TIMEOUT" ]; then
            kill "$capture_pid" 2>/dev/null || true
            wait "$capture_pid" 2>/dev/null || true
            printf '%s\t%s\t%s\tcapture-timeout\n' "$label" "$mode" "$index" >> "$FAILURES"
            return 1
        fi

        if [ $((now - last_write)) -ge "$IDLE_TIMEOUT" ]; then
            kill "$capture_pid" 2>/dev/null || true
            wait "$capture_pid" 2>/dev/null || true
            printf '%s\t%s\t%s\tcapture-idle-timeout\n' "$label" "$mode" "$index" >> "$FAILURES"
            return 1
        fi

        sleep 1
    done

    wait "$capture_pid" 2>/dev/null || true
    if grep -q '\[BENCH\]' "$log" 2>/dev/null; then
        return 0
    fi
    printf '%s\t%s\t%s\tcapture-exit\n' "$label" "$mode" "$index" >> "$FAILURES"
    return 1
}

jobs=()
add_range() {
    local cwe="$1" subdir="$2" mode="$3" first="$4" last="$5" i
    for ((i = first; i <= last; ++i)); do
        jobs+=("$cwe $subdir $mode $i")
    done
}

# Full detector logs that timed out before the first BENCH line.
add_range CWE124_Buffer_Underwrite s03 full 1 2
add_range CWE126_Buffer_Overread s01 full 1 11
add_range CWE126_Buffer_Overread s02 full 1 7
add_range CWE127_Buffer_Underread s02 full 1 11
add_range CWE191_Integer_Underflow s02 full 1 2
add_range CWE191_Integer_Underflow s03 full 1 3
add_range CWE191_Integer_Underflow s04 full 1 5
add_range CWE415_Double_Free s01 full 1 6

# Native baseline logs with timeout or CPU fault in group execution.
add_range CWE127_Buffer_Underread s02 native 1 11
add_range CWE124_Buffer_Underwrite s02 native 1 11
add_range CWE416_Use_After_Free - native 1 7

total="${#jobs[@]}"
status=0
for i in "${!jobs[@]}"; do
    # shellcheck disable=SC2086
    run_one "$((i + 1))" "$total" ${jobs[$i]} || status=1
done

if [ -s "$FAILURES" ]; then
    echo "Remainder microbenchmark completed with failures:"
    cat "$FAILURES"
    exit 1
fi

echo "Remainder microbenchmark completed successfully."
