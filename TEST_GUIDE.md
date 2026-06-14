# 메모리 보호 패스 테스트 가이드

STM32L562 보드에서 LLVM 메모리 보호 패스 적용 전/후 성능 및 탐지율을 측정하는 절차입니다.

---

## 전제 조건

### 소프트웨어
| 도구 | 용도 |
|---|---|
| clang-18 / llvm-18 | LLVM IR 컴파일 및 패스 적용 |
| arm-none-eabi-gcc | ARM 크로스 컴파일 |
| cmake | 패스/런타임 빌드 |
| STM32CubeProgrammer CLI | 펌웨어 플래싱 |
| Python 3.8+ | 테스트 러너 생성, 비교 스크립트 |
| pyserial (선택) | UART 자동 캡처 |

```bash
pip install pyserial   # 선택사항
```

### 하드웨어 연결

```
STM32L562 보드
  ├── ST-Link (CN1 USB) ─────────── PC USB   : 플래싱 (SWD)
  └── USART1
        PA9  (TX) ── USB-UART RX
        GND       ── USB-UART GND
                      PC: /dev/ttyUSB0 (Linux) | COMx (Windows)

설정: 115200 baud / 8N1
```

---

## STEP 1 — LLVM 패스 재빌드

`TagPass.cpp` (Bug 1 수정), `MPUPass.cpp` (Bug 5 수정) 변경사항을 반영합니다.

```bash
cd ~/master-project
cmake --build build/llvm -- -j$(nproc)
```

확인:
```bash
ls -lh build/llvm/libTagPass.so build/llvm/libMPUPass.so
# 수정 시각이 최신인지 확인
```

---

## STEP 2 — 런타임 재빌드

`heapManager.c` 힙 워터마크 추가사항을 반영합니다.

```bash
cmake --build build/runtime -- -j$(nproc)
```

---

## STEP 3 — WITH-PASS 펌웨어 빌드

```bash
cd ~/master-project

# 단일 CWE (처음 테스트 시 권장)
./build_per_cwe.sh CWE121 s01

# 전체 빌드
./build_per_cwe.sh
```

산출물: `firmware/firmware_CWE121_s01.elf` / `.bin`

---

## STEP 4 — WITHOUT-PASS 펌웨어 빌드

`NO_PASS=1` 환경변수를 설정하면 opt-18 패스 단계를 건너뛰고 원본 IR 그대로 컴파일합니다.

```bash
NO_PASS=1 ./build_per_cwe.sh CWE121 s01

# 전체
NO_PASS=1 ./build_per_cwe.sh
```

산출물: `firmware/firmware_CWE121_s01_nopass.elf` / `.bin`

---

## STEP 5 — 정적 크기 비교 (보드 없이)

보드 없이 즉시 Flash / RAM 오버헤드를 확인할 수 있습니다.

```bash
# 단일 쌍
python3 compare_metrics.py \
  --pass-elf   firmware/firmware_CWE121_s01.elf \
  --nopass-elf firmware/firmware_CWE121_s01_nopass.elf

# firmware/ 디렉토리 전체 자동 매칭
python3 compare_metrics.py --firmware-dir firmware/
```

---

## STEP 6 — WITH-PASS 플래시 및 로그 수집

```bash
# 플래시
STM32_Programmer_CLI -c port=SWD \
  -w firmware/firmware_CWE121_s01.bin 0x08000000 -rst

# UART 로그 수집 (보드 리셋 직후 자동 시작됨)
python3 capture_uart.py /dev/ttyUSB0 with_pass.log
```

`capture_uart.py`가 없는 경우 (테스트 완료 후 수동 Ctrl+C):
```bash
stty -F /dev/ttyUSB0 115200 raw
cat /dev/ttyUSB0 | tee with_pass.log
```

---

## STEP 7 — WITHOUT-PASS 플래시 및 로그 수집

```bash
STM32_Programmer_CLI -c port=SWD \
  -w firmware/firmware_CWE121_s01_nopass.bin 0x08000000 -rst

python3 capture_uart.py /dev/ttyUSB0 no_pass.log
```

---

## STEP 8 — 동적 지표 비교

```bash
python3 compare_metrics.py \
  --pass-log   with_pass.log \
  --nopass-log no_pass.log \
  --pass-elf   firmware/firmware_CWE121_s01.elf \
  --nopass-elf firmware/firmware_CWE121_s01_nopass.elf
```

출력 예시:
```
============================================================
  동적 지표 비교 (실행시간 / 힙 / 스택)
============================================================
Test Case                           bad_us  (nopass)   오버헤드  heap_pass heap_nopass  stk_pass stk_nopass
CWE121 (char_alloca_loop_01)           342        87     +293.1%        0B          0B      512B      192B
...
============================================================
  정적 메모리 비교
============================================================
  Flash (text+data)          pass=  98304B  nopass=  61440B  diff= +36864B (+60.0%)
  RAM 정적 (data+bss)        pass=   8192B  nopass=   4096B  diff=  +4096B (+100.0%)
```

---

## STEP 9 — 결과 검증 체크리스트

| 항목 | 기대값 |
|---|---|
| nopass → `bad()` | `FAIL-MISS` (미탐지) |
| pass → `bad()` | `PASS` (탐지) |
| pass → `good()` | `PASS` (오탐 없음) |
| Flash 오버헤드 | +30~60% |
| 실행시간 오버헤드 | +2~5× |
| 힙 peak 차이 | ≈0 (동일해야 함) |
| 스택 peak 오버헤드 | +200~500 B |

---

## 전체 흐름 요약

```
STEP 1  cmake --build build/llvm          ← Bug 1, 5 수정 반영
STEP 2  cmake --build build/runtime       ← 힙 워터마크 반영
STEP 3  ./build_per_cwe.sh                → firmware_*.bin       (with pass)
STEP 4  NO_PASS=1 ./build_per_cwe.sh      → firmware_*_nopass.bin
STEP 5  compare_metrics.py --firmware-dir ← 보드 없이 정적 크기 확인
STEP 6  flash with-pass  → with_pass.log
STEP 7  flash no-pass    → no_pass.log
STEP 8  compare_metrics.py --pass-log ... --nopass-log ...
STEP 9  결과 검증
```

---

## 수정된 버그 요약

### Bug 1 — PointerArithmeticPass 루프 내 GEP 미탐지 (`llvm/TagPass.cpp`)

**원인**: 루프 내 GEP에 대해 ExitBlock predecessor에서 GEP를 찾으려 했으나,
GEP는 루프 바디 BB에만 존재하고 다른 predecessor에는 없어
`validForAllPredecessors = false` → `compare_tag` 미삽입.

**수정**: 루프/비루프 구분 제거, 모든 GEP에 대해 GEP 직후 즉시 `compare_tag` 삽입.

### Bug 5 — GlobalVariableMPUPass fault 복구 후 MPU 재설정 누락 (`llvm/MPUPass.cpp`)

**원인**: `lastGlobalVariable = nullptr` 리셋이 함수 단위로만 수행되어,
`fault_recover_body`가 MPU를 전체 초기화한 뒤 동일 함수 내 다음 BB에서
같은 전역 변수에 재접근할 때 MPU가 재설정되지 않음.

**수정**: `lastGlobalVariable = nullptr` 리셋을 BB 단위로 이동.

---

## 측정 항목 설명

| 항목 | 측정 방법 | 위치 |
|---|---|---|
| 실행시간 | DWT_CYCCNT (CPU 사이클 카운터) | `test_runner.c` |
| 힙 peak | `my_malloc`/`my_free` 워터마크 | `heapManager.c` |
| 스택 peak | 1536B 캐너리 패턴 (0xCC) 스캔 | `test_runner.c` |
| Flash 크기 | `arm-none-eabi-size` text+data | `compare_metrics.py` |
| RAM 정적 | `arm-none-eabi-size` data+bss | `compare_metrics.py` |
