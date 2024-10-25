#include "llvm/Passes/PassPlugin.h"  // 새로운 패스 관리 시스템 헤더
#include "llvm/Passes/PassBuilder.h" // PassBuilder 정의
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h" 
#include "llvm/IR/Module.h"  
#include "llvm/IR/IRBuilder.h" 


using namespace llvm;

namespace {

struct StackMPUPass : public PassInfoMixin<StackMPUPass> {
    // call analysis (push stack frame)
    // return analysis (pop stack frame)
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "StackMPUPass"; }
};

struct HeapMPUPass : public PassInfoMixin<HeapMPUPass> {
    // Find where heap object in-use has been changed
    // locate the redzone of the heap object
    // set mpu config of over/under redzone of the heap object
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "HeapMPUPass"; }
};

struct GlobalVariableMPUPass : public PassInfoMixin<GlobalVariableMPUPass> {
    // Find where global object in-use has been changed
    // locate the redzone of the global object
    // set mpu config of over/under redzone of the global object
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
}; 

} // namespace 