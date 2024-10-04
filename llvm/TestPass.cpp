#include "llvm/Passes/PassPlugin.h"  // 새로운 패스 관리 시스템 헤더
#include "llvm/Passes/PassBuilder.h" // PassBuilder 정의
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "cmsis_armcc.h"

#ifdef STM32_ENV
#include "stm32l5xx_hal.h"
#include "system_stm32l5xx.h"
#include "stm32l5xx_hal_dma.h"
#include "stm32l5xx_hal_uart.h"  // UART 모듈 헤더 파일
#include "main.h"
#else
void uart_send_string(const char *str) {
    // Host 환경에서 UART 없이 동작. 필요시 stderr로 출력하거나 다른 방식 사용.
    fprintf(stderr, "UART: %s\n", str);
}
#endif

using namespace llvm;

namespace {

    struct MyTestPass : public PassInfoMixin<MyTestPass> {
          // 생성자 및 복사 방지
        MyTestPass() = default;
        MyTestPass(const MyTestPass &) = delete;  // 복사 금지
        MyTestPass &operator=(const MyTestPass &) = delete;  // 대입 연산 금지
        MyTestPass(MyTestPass &&) = default;  // 이동 허용
        MyTestPass &operator=(MyTestPass &&) = default;  // 이동 대입 허용



         PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
            std::string functionName = "Function Name: " + F.getName().str() + "\n";
            uart_send_string(functionName.c_str());  // UART로 함수 이름 출력
            return PreservedAnalyses::all();  // 패스가 변경되지 않음을 의미
        }
    };
}



extern "C" ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "MyTestPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "my-test-pass") {
                            FPM.addPass(MyTestPass());
                            return true;
                        }
                        return false;
                    });
            }};
}