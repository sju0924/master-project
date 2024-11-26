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

            if (F.getName() == "__global_var_init" || F.getName() == "uart_send_string" ||  F.getName() == "HAL_UART_Transmit"){
                return PreservedAnalyses::all();
            }
            std::string functionName = "Function Name: " + F.getName().str();
            print_string(functionName.c_str());  // UART로 함수 이름 출력
            
           
            print_string("insert test_print");
            //첫 번째 블록을 선택하여 코드 삽입
            BasicBlock &EntryBlock = F.getEntryBlock();
            IRBuilder<> builder(&EntryBlock.front());
            
                // "runtime_func" 함수의 선언을 모듈에서 찾습니다.
            Function *testPrint = F.getParent()->getFunction("test_print");
            if (!testPrint) {
                // 런타임 함수가 없으면 선언을 추가합니다.
                LLVMContext &context = F.getContext();

                Type *returnType = Type::getVoidTy(context);  // 반환 타입 (i8)
                Type *int8Type = Type::getInt8Ty(context);
                Type *paramType = PointerType::get(int8Type, 0);  // 매개변수 타입 (void)

                std::vector<Type*> params;
                params.push_back(paramType);  // 매개변수 추가

                FunctionType *funcType = FunctionType::get(returnType, params, false);
                testPrint = Function::Create(funcType, Function::ExternalLinkage, "test_print", F.getParent());
                

                // 함수 이름을 가져와서 문자열 리터럴로 변환
                Value *funcName = builder.CreateGlobalStringPtr(F.getName());

                // 함수의 시작 부분에 runtime_func 호출을 삽입
                builder.CreateCall(testPrint, funcName);

            }
            

           

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