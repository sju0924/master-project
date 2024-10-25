#include "MPUPass.h"

PreservedAnalyses StackMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  errs() << "Analyzing function: " << F.getName() << "\n";

    for (auto &BB : F) {
        for (auto &I : BB) {
            // Call analysis: CallInst를 통해 함수 호출 감지
            if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                if (Function *calledFunc = CI->getCalledFunction()) {
                    errs() << "  Function call detected: " 
                           << F.getName() << " calls " 
                           << calledFunc->getName() << "\n";
                } else {
                    errs() << "  Indirect function call detected in function: " 
                           << F.getName() << "\n";
                }
            }

            // Return analysis: ReturnInst를 통해 함수 리턴 감지
            if (isa<ReturnInst>(&I)) {
                errs() << "  Return detected in function: " 
                       << F.getName() << "\n";
            }
        }
    }

    // 모든 분석 정보를 보존하도록 설정
    return PreservedAnalyses::all();
}

PreservedAnalyses HeapMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    // To be implemented
    return PreservedAnalyses::all();
}

PreservedAnalyses GlobalVariableMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    // To be implemented
    return PreservedAnalyses::all();
}


// 패스 플러그인 등록
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "MPUPasses", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "stack-mpu-pass") {
                        FPM.addPass(StackMPUPass());
                        return true;
                    } else if (Name == "heap-mpu-pass") {
                        FPM.addPass(HeapMPUPass());
                        return true;
                    } else if (Name == "global-variable-mpu-pass") {
                        FPM.addPass(GlobalVariableMPUPass());
                        return true;
                    }
                    return false;
                });
        }};
}