#include "llvm/Passes/PassPlugin.h"  // 새로운 패스 관리 시스템 헤더
#include "llvm/Passes/PassBuilder.h" // PassBuilder 정의
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h" 
#include "llvm/IR/Module.h"  
#include "llvm/IR/IRBuilder.h" 

void print_string(const char *str) {
    // Host 환경에서 UART 없이 동작. 필요시 stderr로 출력하거나 다른 방식 사용.
    fprintf(stderr, "UART: %s\n", str);
}

using namespace llvm;

namespace {

    struct MyTestPass : public PassInfoMixin<MyTestPass> {

         PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
            std::string functionName = "Function Name: " + F.getName().str() + "\n";
            print_string(functionName.c_str());  // UART로 함수 이름 출력
            
            //첫 번째 블록을 선택하여 코드 삽입
            BasicBlock &EntryBlock = F.getEntryBlock();
            IRBuilder<> builder(&EntryBlock.front());

            // "runtime_func" 함수의 선언을 모듈에서 찾습니다.
            Function *testPrint = F.getParent()->getFunction("test_print");
            if (!testPrint) {
                // 런타임 함수가 없으면 선언을 추가합니다.
                LLVMContext &context = F.getContext();
                FunctionType *funcType = FunctionType::get(Type::getInt8Ty(context), 0);
                testPrint = Function::Create(funcType, Function::ExternalLinkage, "test_print", F.getParent());
            }

            // 함수 이름을 가져와서 문자열 리터럴로 변환
            LLVMContext &context = F.getContext();
            Value *funcName = builder.CreateGlobalStringPtr(F.getName());

            // 함수의 시작 부분에 runtime_func 호출을 삽입
            builder.CreateCall(testPrint, funcName);

            return PreservedAnalyses::none();  // 함수에 변화가 있음을 표시
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