# Hardware-Assisted Memory Error Detection in Embedded Systems

> **Je-On Sohn**, Master of Engineering, Kyung Hee University (February 2025)  
> Advised by Prof. Jinsung Cho, Ph.D.  
> Department of Computer Science and Engineering

임베디드 시스템(STM32L562, Cortex-M33)에서 **MPU(Memory Protection Unit)** 와 **태그 메모리(Tag Memory)** 를 결합하여 하드웨어 보조 메모리 오류 탐지를 구현한 연구입니다. LLVM 패스를 통해 컴파일 타임에 자동으로 계측 코드를 삽입하여, 런타임 오버헤드를 최소화하면서 스택·힙·전역변수의 오버플로우, use-after-free, null pointer dereference를 탐지합니다.

---

## 동작 원리

이 시스템은 두 가지 보호 메커니즘을 결합합니다.

**MPU 기반 거친 보호 (Coarse-grained)**  
함수 호출/반환, 힙 접근, 전역변수 접근 시 MPU 레드존(redzone)을 동적으로 설정하여 인접 메모리 영역을 접근 불가 상태로 만듭니다. MPU 폴트 발생 시 MemManage Handler가 폴트 주소와 원인을 즉각 수집합니다.

| MPU Region | 대상 |
|-----------|------|
| 0, 1 | 스택 프레임 경계 |
| 2, 3 | 힙 객체 경계 |
| 4, 5 | 전역변수 경계 |
| 6 | 해제된(poisoned) 메모리 |
| 7 | Null pointer (0x0) |

**태그 메모리 기반 세밀한 보호 (Fine-grained)**  
각 메모리 객체(스택 변수, 힙 객체, 구조체 멤버, 전역변수)에 고유 태그를 부여하고, 섀도우 메모리(SRAM `0x2002A000`–`0x20030000`)에 저장합니다. 태그 주소는 `원본 주소 / 8`로 계산됩니다. 위험한 메모리 접근(포인터 연산, memcpy/memset 등)이 감지되면 시작·끝 주소의 태그를 비교하여 경계 위반을 탐지합니다.

---

## 시스템 구조

```
master-project/
├── CMakeLists.txt              # 최상위 빌드 (LLVM 패스 적용 + 최종 링크)
├── llvm/                       # LLVM 패스 소스
│   ├── TagPass.cpp/h           # 메모리 태깅 패스
│   ├── MPUPass.cpp/h           # MPU 설정 삽입 패스
│   └── TestPass.cpp            # 함수 호출 흐름 분석 패스
├── runtime/                    # 타겟 디바이스 런타임 라이브러리
│   ├── MPU.c                   # MPU 제어 함수
│   ├── tagManager.c/h          # 태그 생성/비교/제거
│   ├── heapManager.c/h         # my_malloc / my_free (태그 훅 포함)
│   ├── intrinsicFunction.c     # memcpy/memset 훅
│   └── runtimeConfig.h         # 레드존 크기, 정렬 상수 정의
└── stm32/                      # STM32 펌웨어 소스
    ├── Makefile
    ├── Core/Src/
    │   ├── application.c       # 통합 테스트 애플리케이션
    │   ├── testcase_without_pass.c  # 패스 미적용 비교 테스트
    │   └── testcases/          # Juliet Test Suite 테스트케이스
    │       ├── CWE121_Stack_Based_Buffer_Overflow/
    │       ├── CWE122_Heap_Based_Buffer_Overflow/
    │       ├── CWE124_Buffer_Underwrite/
    │       ├── CWE126_Buffer_Overread/
    │       ├── CWE415_Double_Free/
    │       ├── CWE416_Use_After_Free/
    │       └── CWE476_NULL_Pointer_Dereference/
    └── Drivers/                # STM32L5xx HAL 드라이버
```

---

## 빌드 파이프라인

```
stm32/ C 소스
    │
    ▼ clang-18  (LLVM IR 생성)
drivers.ll   application.ll
    │
    ▼ LLVM 패스 (opt-18)
    │  1. struct-metadata-pass           구조체 크기/오프셋 메타데이터 수집
    │     global-variable-tag-pass       전역변수 레드존 추가 및 태그 초기화 삽입
    │                                    → application_tag_gv_struct_output.ll
    │
    │  2. stack-tag-pass                 스택 변수 태그 설정 삽입
    │     arithmetic-pointer-tag-pass    포인터 연산 시 태그 비교 삽입
    │                                    → application_tag_stack_arith_output.ll
    │
    │  3. my-test-pass                   함수 호출 흐름 분석
    │     stack-mpu-pass                 함수 호출/반환 시 스택 MPU 설정 삽입
    │     heap-mpu-pass                  힙 접근 시 MPU 설정 삽입
    │                                    → application_mpu_heap_stack_analysed.ll
    │
    │  4. global-variable-mpu-pass       전역변수 접근 시 MPU 설정 삽입
    │                                    → application_output.ll
    │
    │  5. null-ptr-mpu-pass              null pointer 방지 MPU 설정 삽입
    │                                    → drivers_analysed.ll
    │
    ▼ llvm-link-18
output.ll
    │
    ▼ llc-18  (ARM 오브젝트 생성)
output.o + startup.o + runtime.o
    │
    ▼ arm-none-eabi-gcc  (최종 링크)
firmware.elf  →  firmware.bin
```

---

## 사전 요구 사항

### 개발 환경 (논문 구현 환경)

| 항목 | 버전 |
|------|------|
| 타겟 보드 | STM32L562 Discovery (Cortex-M33) |
| Flash / SRAM | 512 KB / 256 KB |
| LLVM/Clang | 18.x |
| Arm GNU Toolchain | 13.3.Rel1 |
| GCC | 13.2.0 |
| OS | Ubuntu 20.04 (WSL2 포함) |

### 패키지 설치

```bash
# LLVM/Clang 18
sudo apt-get install -y clang-18 llvm-18 llvm-18-dev

# libc++ (LLVM 패스 빌드용)
sudo apt-get install -y libc++-18-dev libc++abi-18-dev

# ARM 크로스 컴파일러
sudo apt-get install -y gcc-arm-none-eabi
```

---

## 빌드 방법

### 전체 빌드 (한 번에)

```bash
mkdir -p build && cd build
cmake ..
make && make apply_pass && make build
```

### 단계별 빌드

#### 1. CMake 설정

```bash
mkdir -p build && cd build
cmake ..
```

#### 2. LLVM 패스 및 펌웨어 IR 생성

```bash
make
```

LLVM 패스 공유 라이브러리(`libTagPass.so`, `libMPUPass.so`, `libTestPass.so`), 런타임(`runtime.o`), 펌웨어 LLVM IR(`drivers.ll`, `application.ll`)이 생성됩니다.

#### 3. LLVM 패스 적용

```bash
make apply_pass
```

패스가 순서대로 적용되어 최종 `output.ll`이 생성됩니다.

#### 4. 최종 펌웨어 빌드

```bash
make build
```

`build/firmware.elf`, `build/firmware.bin`이 생성됩니다.

---

## 빌드 결과물

| 파일 | 설명 |
|------|------|
| `build/firmware.elf` | 디버그 심볼 포함 ELF 바이너리 (플래싱 및 디버깅용) |
| `build/firmware.bin` | 플래싱용 바이너리 |
| `build/output.ll` | 패스 적용 후 최종 LLVM IR |
| `build/llvm/libTagPass.so` | 태깅 패스 플러그인 |
| `build/llvm/libMPUPass.so` | MPU 패스 플러그인 |
| `build/llvm/libTestPass.so` | 테스트 패스 플러그인 |

---

## LLVM 패스 설명

| 패스 이름 | 소속 | 설명 |
|-----------|------|------|
| `struct-metadata-pass` | TagPass | 구조체 타입별 크기/오프셋 메타데이터를 `struct_member_offsets[]` 등에 수집 |
| `global-variable-tag-pass` | TagPass | 전역변수에 레드존 추가, 초기화 루틴에 `set_tag()` 삽입 |
| `stack-tag-pass` | TagPass | ALLOCA 감지 시 `set_tag()` / `set_struct_tag()` 삽입 |
| `arithmetic-pointer-tag-pass` | TagPass | 포인터 연산 및 반복 접근 시 `compare_tag()` 삽입 |
| `my-test-pass` | TestPass | 함수 호출 흐름 분석 및 UART 출력 삽입 |
| `stack-mpu-pass` | MPUPass | 함수 호출 전후로 `configure_mpu_redzone_for_call/return()` 삽입 |
| `heap-mpu-pass` | MPUPass | 힙 객체 접근 시 `configure_mpu_redzone_for_heap_access()` 삽입 |
| `global-variable-mpu-pass` | MPUPass | 전역변수 접근 시 `configure_mpu_redzone_for_global()` 삽입 |
| `null-ptr-mpu-pass` | MPUPass | `main` 진입 시 `configure_mpu_for_null_ptr()` 삽입 |

---

## 탐지 가능한 메모리 오류 (Juliet Test Suite 기준)

| CWE | 오류 유형 | 탐지 방법 |
|-----|-----------|-----------|
| CWE121 | 스택 기반 버퍼 오버플로우 | MPU 스택 레드존 |
| CWE122 | 힙 기반 버퍼 오버플로우 | MPU 힙 레드존 + 태그 비교 |
| CWE124 | 버퍼 언더플로우 (스택/힙/전역변수) | MPU 레드존 |
| CWE126 | 버퍼 오버리드 (전역변수) | MPU 전역변수 레드존 |
| CWE415 | Double-Free | MPU poison 영역 |
| CWE416 | Use-After-Free | MPU poison 영역 + 태그 비교 |
| CWE476 | Null Pointer Dereference | MPU Region 7 (0x0 주소 차단) |

---

## 성능 오버헤드 (논문 평가 결과)

- **실행 시간**: 테스트케이스당 약 +0.1ms 증가. 단순 케이스는 최대 10배 이상, 복잡한 케이스(CWE124 s02)는 1.3배 수준으로 계산 복잡도가 높을수록 상대적 오버헤드 감소.
- **코드 크기**: 평균 **3.1배** 증가 (최소 2.2배 ~ 최대 4.4배). 전체 펌웨어보다 사용자 애플리케이션 또는 오류 취약 코드 구간에만 선택적으로 패스를 적용할 것을 권장.

---

## 재빌드 시 주의사항

`runtime.o`는 CMake 타임스탬프 문제로 자동 갱신이 되지 않을 수 있습니다. 런타임 소스(`MPU.c`, `tagManager.c` 등) 수정 후에는 수동으로 재생성하세요.

```bash
cd build
arm-none-eabi-ld -r runtime/CMakeFiles/runtime.dir/*.o -o runtime/runtime.o
make build
```
