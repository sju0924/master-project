#!/usr/bin/env bash
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HELPER="/home/jeon/.codex/skills/stm32-cwe-board-test/scripts/cwe_board_test.py"
UART="${UART:-/dev/ttyACM0}"
TIMEOUT="${TIMEOUT:-300}"
IDLE_TIMEOUT="${IDLE_TIMEOUT:-90}"
LOG_DIR="${LOG_DIR:-$ROOT/evaluation_logs}"
CWE_FILTER="${CWE_FILTER:-}"
FAILURES="$LOG_DIR/failures.tsv"

mkdir -p "$LOG_DIR"
: > "$FAILURES"

targets=()
for cwe_path in "$ROOT"/juliet-dynamic/testcases/CWE*; do
    [ -d "$cwe_path" ] || continue
    cwe_name="$(basename "$cwe_path")"
    if [ -n "$CWE_FILTER" ] && [ "$cwe_name" != "$CWE_FILTER" ]; then
        continue
    fi
    mapfile -t subdirs < <(find "$cwe_path" -maxdepth 1 -type d -name 's*' | sort)

    if [ "${#subdirs[@]}" -gt 0 ]; then
        for subdir_path in "${subdirs[@]}"; do
            supported="$(find "$subdir_path" -name '*.c' \
                | grep -v fgets | grep -v socket | grep -v fscanf | grep -v file_ \
                | wc -l)"
            [ "$supported" -gt 0 ] || continue
            targets+=("$cwe_name $(basename "$subdir_path")")
        done
    else
        supported="$(find "$cwe_path" -name '*.c' \
            | grep -v fgets | grep -v socket | grep -v fscanf | grep -v file_ \
            | wc -l)"
        [ "$supported" -gt 0 ] || continue
        targets+=("$cwe_name -")
    fi
done

run_one() {
    local index="$1"
    local total="$2"
    local cwe="$3"
    local subdir="$4"
    local mode="$5"
    local short="${cwe%%_*}"
    local label="$cwe"
    local subdir_args=()

    if [ "$subdir" != "-" ]; then
        label="${label}_${subdir}"
        subdir_args=(--subdir "$subdir")
    fi

    local log="$LOG_DIR/${label}_${mode}.log"
    if grep -q 'Result:' "$log" 2>/dev/null; then
        echo "[$index/$total] SKIP $label $mode (complete log)"
        return 0
    fi

    if ! python3 "$HELPER" preflight --uart "$UART" >/dev/null; then
        printf '%s\t%s\tboard-disconnected\n' "$label" "$mode" >> "$FAILURES"
        return 2
    fi

    echo "[$index/$total] RUN  $label $mode"
    timeout -k 5s "${TIMEOUT}s" \
        python3 "$ROOT/capture_uart.py" "$UART" "$log" --baud 115200 &
    local capture_pid=$!
    sleep 1

    if ! kill -0 "$capture_pid" 2>/dev/null; then
        wait "$capture_pid" 2>/dev/null || true
        printf '%s\t%s\tcapture-open\n' "$label" "$mode" >> "$FAILURES"
        return 2
    fi

    if ! python3 "$HELPER" flash "$short" "${subdir_args[@]}" --mode "$mode"; then
        kill "$capture_pid" 2>/dev/null || true
        wait "$capture_pid" 2>/dev/null || true
        printf '%s\t%s\tflash\n' "$label" "$mode" >> "$FAILURES"
        return 1
    fi

    local start now last_write
    start="$(date +%s)"
    last_write="$start"
    while kill -0 "$capture_pid" 2>/dev/null; do
        if grep -q 'Result:' "$log" 2>/dev/null; then
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
            printf '%s\t%s\tcapture-timeout\n' "$label" "$mode" >> "$FAILURES"
            return 1
        fi

        if [ $((now - last_write)) -ge "$IDLE_TIMEOUT" ]; then
            kill "$capture_pid" 2>/dev/null || true
            wait "$capture_pid" 2>/dev/null || true
            printf '%s\t%s\tcapture-idle-timeout\n' "$label" "$mode" >> "$FAILURES"
            return 1
        fi

        sleep 1
    done

    if ! wait "$capture_pid"; then
        printf '%s\t%s\tcapture-timeout\n' "$label" "$mode" >> "$FAILURES"
        return 1
    fi

    if ! grep -q 'Result:' "$log"; then
        printf '%s\t%s\tmissing-result\n' "$label" "$mode" >> "$FAILURES"
        return 1
    fi
}

total=$((${#targets[@]} * 2))
index=0
for target in "${targets[@]}"; do
    read -r cwe subdir <<< "$target"
    for mode in pass nopass; do
        index=$((index + 1))
        run_one "$index" "$total" "$cwe" "$subdir" "$mode"
        result=$?
        if [ "$result" -eq 2 ]; then
            echo "Board connection lost; stopping suite."
            exit 2
        fi
    done
done

if [ -s "$FAILURES" ]; then
    echo "Suite completed with failures:"
    cat "$FAILURES"
    exit 1
fi

echo "Suite completed: $total/$total logs contain Result:"
