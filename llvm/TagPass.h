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
#include "llvm/Analysis/LoopInfo.h"
#include <set>


using namespace llvm;

namespace {

bool isUserDefinedStruct(StructType *structType);

struct StackTagPass : public PassInfoMixin<StackTagPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "StackTagPass"; }
};

struct GlobalVariableTagPass : public PassInfoMixin<GlobalVariableTagPass> {
public: 

    PreservedAnalyses run(Module &F, ModuleAnalysisManager &AM);
    static StringRef name() { return "GlobalVariableTagPass"; }

private:
 
};

struct PointerArithmeticPass : public PassInfoMixin<GlobalVariableTagPass> {
public: 

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "PointerArithmeticPass"; }

private:
 
}; 

struct StructMetadataPass : public PassInfoMixin<GlobalVariableTagPass> {
public: 

    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    static StringRef name() { return "StructMetadataPass"; }

private:

     // 각 멤버의 오프셋과 크기 정보를 수집하여 전역 변수에 저장
    void collectStructMetadata(StructType *structType, const DataLayout &DL, LLVMContext &Context,
                               std::vector<Constant *> &offsetsArray, std::vector<Constant *> &sizesArray);

    // malloc 호출에서 크기를 기준으로 구조체 타입인지 추정
    StructType* detectMallocStructType(CallInst *callInst, const DataLayout &DL);
 
}; 

} // namespace 