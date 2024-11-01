#include "llvm/Passes/PassPlugin.h"  // 새로운 패스 관리 시스템 헤더
#include "llvm/Passes/PassBuilder.h" // PassBuilder 정의
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h" 
#include "llvm/IR/Module.h"  
#include "llvm/IR/IRBuilder.h" 
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Transforms/Utils/ModuleUtils.h" 
#include <set>


using namespace llvm;

namespace {

struct StackTagPass : public PassInfoMixin<StackTagPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "StackTagPass"; }
};

struct GlobalVariableTagPass : public PassInfoMixin<GlobalVariableTagPass> {
public: 

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "GlobalVariableTagPass"; }

private:
 
}; 

} // namespace 