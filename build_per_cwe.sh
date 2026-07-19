#!/usr/bin/env bash
set -e

PROJ_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$PROJ_ROOT/build"
STM32_DIR="$PROJ_ROOT/stm32"
JULIET_DIR="$PROJ_ROOT/juliet-dynamic/testcases"
OUTPUT_DIR="$PROJ_ROOT/firmware"
PASS_BUILD_DIR="$BUILD_DIR/pass_plugins"
PASS_DIR="$PASS_BUILD_DIR"

mkdir -p "$OUTPUT_DIR"

# ── 헬퍼 함수 ──────────────────────────────────────────────────────────
run_passes() {
    local app_ll="$1"

    if [ "${NO_PASS:-0}" = "1" ] || [ "${NATIVE_BASELINE:-0}" = "1" ]; then
        echo "    [pass] SKIP (baseline mode)"
        cp "$app_ll" "$BUILD_DIR/application_output.ll"
        return
    fi

    echo "    [pass 1/5] integer-underflow..."
    opt-18 --load-pass-plugin="$PASS_DIR/libIntegerUnderflowPass.so" \
        -passes="integer-underflow-pass" \
        "$app_ll" -o "$BUILD_DIR/p0.ll" 2>/dev/null

    echo "    [pass 2/5] struct/global-tag..."
    opt-18 --load-pass-plugin="$PASS_DIR/libTagPass.so" \
        -passes="struct-metadata-pass,global-variable-tag-pass" \
        "$BUILD_DIR/p0.ll" -o "$BUILD_DIR/p1.ll" 2>/dev/null

    echo "    [pass 3/5] stack/arith-tag..."
    opt-18 --load-pass-plugin="$PASS_DIR/libTagPass.so" \
        -passes="stack-tag-pass,arithmetic-pointer-tag-pass" \
        "$BUILD_DIR/p1.ll" -o "$BUILD_DIR/p2.ll" 2>/dev/null

    echo "    [pass 4/5] test/stack/heap-mpu..."
    opt-18 --load-pass-plugin="$PASS_DIR/libTestPass.so" \
        --load-pass-plugin="$PASS_DIR/libMPUPass.so" \
        -passes="my-test-pass,stack-mpu-pass,heap-mpu-pass" \
        "$BUILD_DIR/p2.ll" -o "$BUILD_DIR/p3.ll" 2>/dev/null

    echo "    [pass 5/5] global-mpu..."
    opt-18 --load-pass-plugin="$PASS_DIR/libMPUPass.so" \
        -passes="global-variable-mpu-pass" \
        "$BUILD_DIR/p3.ll" -o "$BUILD_DIR/application_output.ll" 2>/dev/null
}

link_and_compile() {
    local label="$1"
    local drivers_ll="${2:-$BUILD_DIR/drivers_analysed.ll}"
    local extra_objects=()
    local runner_defines=()

    if [ "${BENCHMARK_MODE:-0}" = "1" ]; then
        runner_defines+=(-DBENCHMARK_MODE)
        if [ -n "${BENCHMARK_WARMUP:-}" ]; then
            runner_defines+=("-DBENCHMARK_WARMUP=${BENCHMARK_WARMUP}")
        fi
        if [ -n "${BENCHMARK_SAMPLES:-}" ]; then
            runner_defines+=("-DBENCHMARK_SAMPLES=${BENCHMARK_SAMPLES}")
        fi
    fi

    echo "    [link] output.ll..."
    llvm-link-18 "$BUILD_DIR/application_output.ll" "$drivers_ll" \
        -o "$BUILD_DIR/output.ll"

    echo "    [llc] output.s..."
    llc-18 -filetype=asm -march=arm -mcpu=cortex-m33 --float-abi=hard \
        "$BUILD_DIR/output.ll" -o "$BUILD_DIR/output.s"

    echo "    [fix_sp] fixed_output.s..."
    python3 "$PROJ_ROOT/tool/fix_sp.py" \
        "$BUILD_DIR/output.s" "$BUILD_DIR/fixed_output.s"

    echo "    [gcc] output.o..."
    arm-none-eabi-gcc -c \
        -mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
        "$BUILD_DIR/fixed_output.s" -o "$BUILD_DIR/output.o"

    echo "    [gcc] test_runner.o..."
    arm-none-eabi-gcc -c \
        -mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
        -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DSTM32L562xx \
        "${runner_defines[@]}" \
        -I"$PROJ_ROOT/stm32/Core/Inc" \
        -I"$PROJ_ROOT/stm32/Core/Inc/testcasesupport" \
        -I"$PROJ_ROOT/stm32/Drivers/STM32L5xx_HAL_Driver/Inc" \
        -I"$PROJ_ROOT/stm32/Drivers/STM32L5xx_HAL_Driver/Inc/Legacy" \
        -I"$PROJ_ROOT/stm32/Drivers/CMSIS/Device/ST/STM32L5xx/Include" \
        -I"$PROJ_ROOT/stm32/Drivers/CMSIS/Include" \
        "$PROJ_ROOT/stm32/Core/Src/test_runner.c" \
        -o "$BUILD_DIR/test_runner.o"

    if [ "${NO_PASS:-0}" = "1" ]; then
        echo "    [gcc] pass_stubs.o..."
        arm-none-eabi-gcc -c \
            -mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
            -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DSTM32L562xx \
            -I"$PROJ_ROOT/runtime" \
            "$PROJ_ROOT/stm32/Core/Src/pass_stubs.c" \
            -o "$BUILD_DIR/pass_stubs.o"
        extra_objects+=("$BUILD_DIR/pass_stubs.o")
    fi

    local runtime_object="$BUILD_DIR/runtime/runtime.o"
    if [ "${NATIVE_BASELINE:-0}" = "1" ]; then
        runtime_object="$BUILD_DIR/runtime/native_runtime.o"
    fi

    echo "    [ld] firmware_${label}.elf..."
    arm-none-eabi-gcc \
        "$BUILD_DIR/output.o" \
        "$BUILD_DIR/startup.o" \
        "$runtime_object" \
        "$BUILD_DIR/test_runner.o" \
        "${extra_objects[@]}" \
        -o "$OUTPUT_DIR/firmware_${label}.elf" \
        -T "$STM32_DIR/STM32L562xE_SHADOW.ld" \
        -mcpu=cortex-m33 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
        -fno-short-enums --specs=nosys.specs

    arm-none-eabi-objcopy -O binary \
        "$OUTPUT_DIR/firmware_${label}.elf" \
        "$OUTPUT_DIR/firmware_${label}.bin"

    local size text data bss
    read text data bss < <(arm-none-eabi-size "$OUTPUT_DIR/firmware_${label}.elf" \
        | tail -1 | awk '{print $1, $2, $3}')
    local flash=$((text + data))
    echo "    => firmware_${label}.bin  flash: ${flash} / 524288 bytes"
    if [ "$flash" -gt 524288 ]; then
        echo "    [WARNING] 512KB 초과! 서브디렉터리를 더 세분화하세요."
    fi
}

rebuild_runtime() {
    local runtime_build="$BUILD_DIR/runtime/CMakeFiles/runtime.dir"
    local runtime_sources=(
        testPrint.c
        MPU.c
        debugger.c
        tagManager.c
        heapManager.c
        intrinsicFunction.c
        syscalls.c
    )
    local runtime_objects=()

    mkdir -p "$runtime_build" "$BUILD_DIR/runtime"

    for src in "${runtime_sources[@]}"; do
        local obj="$runtime_build/${src}.o"
        arm-none-eabi-gcc -c \
            -mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs \
            -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DSTM32L562xx \
            -I"$PROJ_ROOT/runtime" \
            -I"$PROJ_ROOT/stm32/Core/Inc" \
            -I"$PROJ_ROOT/stm32/Drivers/STM32L5xx_HAL_Driver/Inc" \
            -I"$PROJ_ROOT/stm32/Drivers/STM32L5xx_HAL_Driver/Inc/Legacy" \
            -I"$PROJ_ROOT/stm32/Drivers/CMSIS/Device/ST/STM32L5xx/Include" \
            -I"$PROJ_ROOT/stm32/Drivers/CMSIS/Include" \
            "$PROJ_ROOT/runtime/$src" \
            -o "$obj"
        runtime_objects+=("$obj")
    done

    arm-none-eabi-ld -r "${runtime_objects[@]}" -o "$BUILD_DIR/runtime/runtime.o"
}

rebuild_native_runtime() {
    local native_build="$BUILD_DIR/runtime/native"
    mkdir -p "$native_build" "$BUILD_DIR/runtime"

    arm-none-eabi-gcc -c \
        -mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs \
        -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DSTM32L562xx \
        -I"$PROJ_ROOT/runtime" \
        -I"$PROJ_ROOT/stm32/Core/Inc" \
        "$PROJ_ROOT/runtime/native_stubs.c" \
        -o "$native_build/native_stubs.o"
    arm-none-eabi-gcc -c \
        -mcpu=cortex-m33 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -specs=nano.specs \
        "$PROJ_ROOT/runtime/syscalls.c" \
        -o "$native_build/syscalls.o"
    arm-none-eabi-ld -r \
        "$native_build/native_stubs.o" "$native_build/syscalls.o" \
        -o "$BUILD_DIR/runtime/native_runtime.o"
}

rebuild_pass_plugins() {
    cmake -S "$PROJ_ROOT/llvm" -B "$PASS_BUILD_DIR" \
        -DCMAKE_C_COMPILER=clang-18 \
        -DCMAKE_CXX_COMPILER=clang++-18 >/dev/null

    cmake --build "$PASS_BUILD_DIR" \
        --target TagPass MPUPass TestPass IntegerUnderflowPass \
        --parallel 8 >/dev/null
}

# ── 1회성 준비 ────────────────────────────────────────────────────────
echo "==> LLVM pass plugin 갱신..."
rebuild_pass_plugins

# Keep the shared driver IR in sync with the STM32 sources.  Per-CWE builds
# previously rebuilt only application.ll, so changes to exception handlers
# (including HardFault recovery) could be silently omitted from the firmware.
echo "==> 공통 STM32 드라이버 IR 갱신..."
rm -f "$STM32_DIR/build/io.ll" "$STM32_DIR/build/drivers.ll"
make -C "$STM32_DIR" -j8 BENCHMARK_MODE="${BENCHMARK_MODE:-0}" build/drivers.ll >/dev/null
if [ "${NATIVE_BASELINE:-0}" = "1" ]; then
    echo "==> Native baseline stubs 갱신..."
    rebuild_native_runtime
else
    echo "==> Runtime object 갱신..."
    rebuild_runtime
fi

cd "$BUILD_DIR"

if [ "${NO_PASS:-0}" = "1" ] || [ "${NATIVE_BASELINE:-0}" = "1" ]; then
    DRIVERS_LL="$STM32_DIR/drivers.ll"
    echo "==> Baseline 모드: drivers.ll 원본 사용"
else
    if [ ! -f drivers_analysed.ll ] || [ "$STM32_DIR/drivers.ll" -nt drivers_analysed.ll ]; then
        echo "==> Generating drivers_analysed.ll (one-time)..."
        opt-18 --load-pass-plugin="$PASS_DIR/libMPUPass.so" \
            -passes="null-ptr-mpu-pass" \
            "$STM32_DIR/drivers.ll" -o drivers_analysed.ll 2>/dev/null
    fi
    DRIVERS_LL="$BUILD_DIR/drivers_analysed.ll"
fi

if [ ! -f startup.o ]; then
    arm-none-eabi-gcc -c "$STM32_DIR/startup_stm32l562xx.s" -o startup.o
fi

# ── 빌드 대상 목록 생성 ───────────────────────────────────────────────
# 형식: "CWE_DIR SUBDIR(없으면 -)"
TARGETS=()
for cwe_path in "$JULIET_DIR"/CWE*; do
    [ -d "$cwe_path" ] || continue
    cwe_name=$(basename "$cwe_path")

    # 파일이 없는 CWE 건너뜀
    n_files=$(find "$cwe_path" -name "*.c" | grep -v fgets | grep -v socket | grep -v fscanf | grep -v file_ | wc -l)
    [ "$n_files" -eq 0 ] && continue

    mapfile -t subdirs < <(find "$cwe_path" -maxdepth 1 -type d -name 's*' 2>/dev/null | sort)
    if [ ${#subdirs[@]} -gt 0 ]; then
        for sd_path in "${subdirs[@]}"; do
            sd=$(basename "$sd_path")
            n=$(find "$sd_path" -name "*.c" | grep -v fgets | grep -v socket | grep -v fscanf | grep -v file_ | wc -l)
            [ "$n" -eq 0 ] && continue
            TARGETS+=("$cwe_name $sd")
        done
    else
        TARGETS+=("$cwe_name -")
    fi
done

echo "==> 총 빌드 대상: ${#TARGETS[@]} 개"
echo ""

# 단일 지정 모드: ./build_per_cwe.sh CWE121 s02
if [ -n "$1" ]; then
    if [ -n "$2" ]; then
        TARGETS=("$1 $2")
    else
        mapfile -t TARGETS < <(printf '%s\n' "${TARGETS[@]}" | grep "^$1")
    fi
    echo "==> 지정 빌드: ${TARGETS[*]}"
fi

# ── 빌드 루프 ─────────────────────────────────────────────────────────
idx=0
for target in "${TARGETS[@]}"; do
    idx=$((idx + 1))
    cwe_name=$(echo "$target" | awk '{print $1}')
    subdir=$(echo "$target" | awk '{print $2}')
    [ "$subdir" = "-" ] && subdir=""

    label="${cwe_name}"
    [ -n "$subdir" ] && label="${cwe_name}_${subdir}"
    if [ "${BENCHMARK_MODE:-0}" = "1" ]; then
        label="${label}_bench"
    fi
    if [ "${NATIVE_BASELINE:-0}" = "1" ]; then
        label="${label}_native"
    elif [ "${NO_PASS:-0}" = "1" ]; then
        label="${label}_nopass"
    fi

    echo "========================================"
    echo "  [$idx/${#TARGETS[@]}] $label"
    echo "========================================"

    # 1. test_runner.c 생성
    echo "  [1] test_runner.c 생성..."
    cd "$PROJ_ROOT"
    runner_range_args=(--start-index "${TEST_START_INDEX:-1}")
    if [ -n "${TEST_END_INDEX:-}" ]; then
        runner_range_args+=(--end-index "$TEST_END_INDEX")
    fi
    if [ -n "$subdir" ]; then
        python3 generate_test_runner.py --cwe "$cwe_name" --subdir "$subdir" \
            "${runner_range_args[@]}" 2>/dev/null
    else
        python3 generate_test_runner.py --cwe "$cwe_name" \
            "${runner_range_args[@]}" 2>/dev/null
    fi
    tc_count=$(grep -c "_bad(void);" "$STM32_DIR/Core/Src/test_runner.c" || echo 0)
    echo "    => ${tc_count} 테스트케이스"

    # 2. application.ll 생성 (이전 타깃의 캐시 방지: application/ 초기화)
    echo "  [2] LLVM IR 컴파일..."
    rm -rf "$STM32_DIR/build/application" "$STM32_DIR/build/application.ll"
    mkdir -p "$STM32_DIR/build/application"
    cd "$STM32_DIR"
    make_mode_args=(
        BENCHMARK_MODE="${BENCHMARK_MODE:-0}"
        NATIVE_BASELINE="${NATIVE_BASELINE:-0}"
    )
    if [ -n "$subdir" ]; then
        make -j8 "${make_mode_args[@]}" BUILD_CWE="$cwe_name" BUILD_SUBDIR="$subdir" build/application.ll \
            2>&1 | grep -E "^llvm-link|error:" | tail -3
    else
        make -j8 "${make_mode_args[@]}" BUILD_CWE="$cwe_name" build/application.ll \
            2>&1 | grep -E "^llvm-link|error:" | tail -3
    fi
    cd "$BUILD_DIR"

    # 3. passes
    echo "  [3] LLVM passes..."
    run_passes "$STM32_DIR/build/application.ll"

    # 4. link & compile
    echo "  [4] 링크 및 컴파일..."
    link_and_compile "$label" "$DRIVERS_LL"

    echo ""
done

echo "========================================"
echo "  전체 빌드 완료"
echo "========================================"
ls -lh "$OUTPUT_DIR"/*.bin 2>/dev/null | awk '{print $5, $9}'
