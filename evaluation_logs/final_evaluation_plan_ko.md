# STM32 메모리 오류 탐지기 최종 평가 방안

## 1. 평가 목적

최종 평가는 다음 두 질문을 분리해 답한다.

1. 탐지기가 없는 native 실행과 비교했을 때 전체 시스템 비용은 얼마인가?
2. runtime-only 기준선과 비교했을 때 LLVM 계측이 독립적으로 탐지한 오류는
   몇 개인가?

CPU fault 또는 runtime-only에서도 동일하게 발생하는 탐지는 LLVM 패스의
성공으로 계산하지 않는다.

## 2. 펌웨어 모드

각 CWE 그룹은 동일 소스와 러너로 다음 세 모드를 빌드한다.

| 모드 | LLVM 패스 | API runtime 치환 | 용도 |
| --- | --- | --- | --- |
| native | 없음 | 없음 | 플랫폼 및 원본 프로그램 기준선 |
| runtime-only | 없음 | 있음 | 런타임 검사 비용과 탐지 분리 |
| full | 있음 | 있음 | 최종 탐지기 |

일반 탐지 결과와 벤치마크 결과는 서로 다른 ELF로 유지한다.

```bash
./build_per_cwe.sh CWE191_Integer_Underflow s01
NO_PASS=1 ./build_per_cwe.sh CWE191_Integer_Underflow s01
NATIVE_BASELINE=1 ./build_per_cwe.sh CWE191_Integer_Underflow s01

BENCHMARK_MODE=1 ./build_per_cwe.sh CWE191_Integer_Underflow s01
NO_PASS=1 BENCHMARK_MODE=1 ./build_per_cwe.sh CWE191_Integer_Underflow s01
NATIVE_BASELINE=1 BENCHMARK_MODE=1 ./build_per_cwe.sh CWE191_Integer_Underflow s01
```

## 3. 탐지 판정

러너는 각 경로에 다음 탐지 출처를 기록한다.

| 출처 | 의미 | 탐지기 성공 인정 |
| --- | --- | --- |
| `software-tag` | 태그 불일치 또는 UAF 검사 | 조건부 인정 |
| `null` | LLVM이 삽입한 NULL 검사 | 조건부 인정 |
| `integer` | 정수 언더플로 검사 | 조건부 인정 |
| `mpu` | MPU 보호 영역 fault | 조건부 인정 |
| `cpu-fault` | 일반 HardFault 등 CPU 자체 fault | 인정하지 않음 |
| `none` | 탐지 없음 | 인정하지 않음 |

최종 성공인 `ATTRIBUTED-PASS`는 다음 조건을 모두 만족해야 한다.

1. full bad 경로의 출처가 `software-tag`, `null`, `integer`, `mpu` 중 하나다.
2. full good 경로의 출처가 `none`이다.
3. runtime-only bad 경로의 출처가 `none`이다.
4. native bad 경로의 출처가 `none`이다.

runtime-only 또는 native에서도 오류가 관찰되면 `BASELINE-DETECTED`, full에서
CPU fault만 발생하면 `CPU-FAULT-ONLY`로 분류한다. 두 상태는 평가 분모에는
포함하지만 성공 건수에는 포함하지 않는다.

```bash
python3 evaluation_logs/evaluate_detector_results.py \
  --pass-log evaluation_logs/CWE191_s01_full.log \
  --runtime-log evaluation_logs/CWE191_s01_runtime.log \
  --native-log evaluation_logs/CWE191_s01_native.log \
  --csv evaluation_logs/CWE191_s01_attribution.csv
```

`evaluation_exclusions.txt`에는 비결정 입력과 8바이트 미만 고정 침범에 대한
공통 제외 정규식이 저장되어 있다.

## 4. 순수 실행시간 측정

`BENCHMARK_MODE`는 good 경로만 측정하며 다음 작업은 DWT 측정 구간 밖에서
수행한다.

- `setjmp`
- MPU, heap, tag 초기화
- stack canary 설정과 측정
- 결과 포맷팅 및 UART 출력

각 테스트는 20회 워밍업 후 200회 측정한다. raw cycle의 minimum, median,
p95, maximum, mean, 합계와 제곱합을 기록한다. 출력 sink는 UART 대신
volatile checksum을 사용한다.

```bash
python3 evaluation_logs/analyze_microbenchmark.py \
  --full evaluation_logs/CWE191_s01_bench_full.log \
  --runtime evaluation_logs/CWE191_s01_bench_runtime.log \
  --native evaluation_logs/CWE191_s01_bench_native.log
```

주 실행시간 지표는 테스트별 `full/native`와 `full/runtime-only` median
비율의 기하평균이다. 변동계수가 5%를 넘는 테스트는
`BENCHMARK_SAMPLES=1000`으로 다시 빌드하고 재실행한다.

```bash
BENCHMARK_MODE=1 BENCHMARK_SAMPLES=1000 \
  ./build_per_cwe.sh CWE191_Integer_Underflow s01
```

오류 경로는 성공률 평가에서 한 번 실행한다. 탐지 조건이 성립한 순간 또는
fault handler 진입 순간의 raw cycle을 `bad_cycles`로 기록하며, UART 출력과
`longjmp` 복구 시간은 탐지 지연에서 제외한다.

## 5. 메모리 비용

정적 비용은 일반 평가 ELF에서 계산한다. 벤치마크 샘플 배열은 탐지기
메모리 비용에 포함하지 않는다.

- Flash: `text + data`
- 정적 RAM: `data + bss`
- 동적 heap: 테스트별 `heap_get_peak()`
- 동적 stack: 별도 1회 실행의 stack high-water

다음 두 차이를 모두 보고한다.

- `full - native`: 전체 탐지 시스템 비용
- `full - runtime-only`: LLVM 계측의 추가 비용

## 6. 전체 실행 순서

1. CWE191 s01 세 모드에서 생성 로그, cycle 분포, 탐지 출처를 검증한다.
2. CWE126 s01에서 checksum sink가 문자열 읽기를 유지하는지 확인한다.
3. 36개 그룹의 일반 full/runtime-only/native 로그를 수집한다.
4. 동일 36개 그룹의 벤치마크 세 모드 로그를 수집한다.
5. 제외 기준을 공통 적용하고 `ATTRIBUTED-PASS`만 성공으로 집계한다.
6. CV 5% 초과 테스트만 1,000회로 재실행한다.
7. CWE별 성공률, 전체 가중 성공률, 오탐률, baseline-detected 비율을 보고한다.
8. 실행시간은 기하평균, median, p95를 보고하고 메모리는 평균과 최댓값을
   함께 보고한다.

최종 논문 표에서는 기존 `PASS`와 새 `ATTRIBUTED-PASS`를 혼용하지 않는다.
새 기준으로 전체 보드 로그를 다시 수집한 뒤 성공률을 확정한다.

## 7. 초기 보드 검증 결과

2026-07-19에 STM32L562E-DK에서 다음 sanity check를 완료했다.

- CWE191 s01 일반 평가: full/runtime-only/native 세 모드 로그를 수집했고,
  `rand` 제외 후 `ATTRIBUTED-PASS`는 2/4이다. 두 성공 항목은
  `int64_t_min_multiply_01`, `int64_t_min_sub_01`이며, full에서만
  `integer` 출처로 탐지되었다.
- CWE191 s01 순수 good-path benchmark: `rand` 제외 후 4개 항목의
  기하평균 오버헤드는 full/native 6.457x, full/runtime-only 6.447x이다.
  CV 5% 초과 항목은 없다.
- CWE126 s01 benchmark: `rand`와 CWE129 제외 후 11개 항목이 모두 fault 없이
  완료되었다. 문자열 loop 계열이 수만 cycle로 측정되어 benchmark sink가
  출력 최적화 제거를 방지하고 있음을 확인했다. 기하평균 오버헤드는
  full/native 4.355x, full/runtime-only 3.576x이며, CV 5% 초과 항목은 없다.

초기 검증 로그는 `evaluation_logs/strict_cwe191_s01_*.log`,
`evaluation_logs/bench_cwe191_s01_*.log`,
`evaluation_logs/bench_cwe126_s01_*.log`에 저장되어 있다.
