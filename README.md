# Master Project

STM32L562xx(Cortex-M33) 타겟의 메모리 안전성 강화를 위한 LLVM 패스 기반 빌드 시스템입니다.  
LLVM IR 수준에서 태깅(Tagging)과 MPU(Memory Protection Unit) 설정을 자동으로 삽입하여 스택/힙/전역변수 오버플로우, use-after-free, null pointer dereference를 탐지합니다.

## 프로젝트 구조

```
master-project/
├── CMakeLists.txt          # 최상위 빌드 파일 (LLVM 패스 적용 및 최종 링크)
├── llvm/                   # LLVM 패스 소스
│   ├── TagPass.cpp/h       # 메모리 태깅 패스 (스택, 힙, 전역변수, 구조체)
│   ├── MPUPass.cpp/h       # MPU 설정 삽입 패스 (스택, 힙, 전역변수, null ptr)
│   └── TestPass.cpp        # 함수 호출 분석 패스
├── runtime/                # 타겟 디바이스에서 실행되는 런타임 라이브러리
│   ├── MPU.c               # MPU 설정 함수 구현
│   ├── tagManager.c/h      # 태그 관리
│   ├── heapManager.c/h     # 힙 관리 (my_malloc/my_free)
│   ├── intrinsicFunction.c # 태그 비교/설정 intrinsic
│   ├── debugger.c          # 디버그 출력
│   └── testPrint.c         # UART 테스트 출력
└── stm32/                  # STM32 펌웨어 소스 (HAL 드라이버, 애플리케이션)
    ├── Makefile
    ├── Core/Src/
    │   ├── application.c   # 테스트 애플리케이션
    │   └── main.c
    └── Drivers/            # STM32L5xx HAL 드라이버
```

## 빌드 파이프라인

```
stm32/ C 소스
    │
    ▼ clang-18 (LLVM IR 생성)
application.ll, drivers.ll
    │
    ▼ LLVM 패스 (opt-18)
    │  1. struct-metadata-pass, global-variable-tag-pass  → application_tag_gv_struct_output.ll
    │  2. stack-tag-pass, arithmetic-pointer-tag-pass     → application_tag_stack_arith_output.ll
    │  3. my-test-pass, stack-mpu-pass, heap-mpu-pass     → application_mpu_heap_stack_analysed.ll
    │  4. global-variable-mpu-pass                        → application_output.ll
    │  5. null-ptr-mpu-pass                               → drivers_analysed.ll
    │
    ▼ llvm-link-18
output.ll
    │
    ▼ llc-18 (ARM 오브젝트 생성)
output.o + startup.o + runtime.o
    │
    ▼ arm-none-eabi-gcc (링크)
firmware.elf → firmware.bin
```

## 사전 요구 사항

### 필수 패키지 설치

```bash
# LLVM/Clang 18
sudo apt-get install -y clang-18 llvm-18 llvm-18-dev

# libc++ (LLVM 패스 빌드용)
sudo apt-get install -y libc++-18-dev libc++abi-18-dev

# ARM 크로스 컴파일러
sudo apt-get install -y gcc-arm-none-eabi
```

### 버전 확인

```bash
clang-18 --version      # Clang 18.x 이상
opt-18 --version
llc-18 --version
arm-none-eabi-gcc --version
```

## 빌드 방법

### 1. 빌드 디렉터리 생성 및 CMake 설정

```bash
mkdir -p build && cd build
cmake ..
```

### 2. 펌웨어 및 LLVM 패스 빌드

```bash
make
```

LLVM 패스(`libTagPass.so`, `libMPUPass.so`, `libTestPass.so`)와 런타임(`runtime.o`), 펌웨어 LLVM IR(`drivers.ll`, `application.ll`)이 생성됩니다.

### 3. LLVM 패스 적용

```bash
make apply_pass
```

각 패스가 순서대로 적용되어 `application_output.ll`, `drivers_analysed.ll`, `output.ll`이 생성됩니다.

### 4. 최종 펌웨어 빌드

```bash
make build
```

`firmware.elf`와 `firmware.bin`이 `build/` 디렉터리에 생성됩니다.

### 전체 빌드 한 번에

```bash
mkdir -p build && cd build
cmake ..
make && make apply_pass && make build
```

## 빌드 결과물

| 파일 | 설명 |
|------|------|
| `build/firmware.elf` | 디버그 심볼 포함 ELF 바이너리 |
| `build/firmware.bin` | 플래싱용 바이너리 |
| `build/output.ll` | 패스 적용 후 최종 LLVM IR |
| `build/llvm/libTagPass.so` | 태깅 패스 플러그인 |
| `build/llvm/libMPUPass.so` | MPU 패스 플러그인 |
| `build/llvm/libTestPass.so` | 테스트 패스 플러그인 |

## LLVM 패스 설명

| 패스 이름 | 소속 | 설명 |
|-----------|------|------|
| `struct-metadata-pass` | TagPass | 구조체 타입 크기/오프셋 메타데이터 수집 |
| `global-variable-tag-pass` | TagPass | 전역변수에 레드존 추가 및 태그 초기화 삽입 |
| `stack-tag-pass` | TagPass | 스택 지역변수 태그 설정 삽입 |
| `arithmetic-pointer-tag-pass` | TagPass | 포인터 연산 시 태그 비교 삽입 |
| `my-test-pass` | TestPass | 함수 호출 흐름 분석 및 UART 출력 삽입 |
| `stack-mpu-pass` | MPUPass | 함수 진입/반환 시 스택 MPU 설정 삽입 |
| `heap-mpu-pass` | MPUPass | 힙 접근 시 MPU 설정 삽입 |
| `global-variable-mpu-pass` | MPUPass | 전역변수 접근 시 MPU 설정 삽입 |
| `null-ptr-mpu-pass` | MPUPass | null pointer dereference 방지 MPU 설정 삽입 |

## 재빌드 시 주의사항

`runtime.o`는 CMake의 `add_custom_command` 타임스탬프 문제로 갱신되지 않을 수 있습니다.  
런타임 소스를 수정한 경우 수동으로 재생성하세요.

```bash
cd build
arm-none-eabi-ld -r runtime/CMakeFiles/runtime.dir/*.o -o runtime/runtime.o
make build
```
