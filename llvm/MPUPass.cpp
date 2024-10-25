#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

struct StackMPUPass : public PassInfoMixin<StackMPUPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

struct HeapMPUPass : public PassInfoMixin<HeapMPUPass> {
    static char ID;
    SecondPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        errs() << "Running SecondPass on function: " << F.getName() << "\n";
        return false;
    }
};

struct GVMPUPass : public PassInfoMixin<GVMPUPass> {
    static char ID;
    SecondPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        errs() << "Running SecondPass on function: " << F.getName() << "\n";
        return false;
    }
};

PreservedAnalyses StackMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  errs() << F.getName() << "\n";
  return PreservedAnalyses::all();
}

} // namespace

char FirstPass::ID = 0;
char SecondPass::ID = 0;

// 패스 등록
static RegisterPass<FirstPass> X("first-pass", "First Pass Example", false, false);
static RegisterPass<SecondPass> Y("second-pass", "Second Pass Example", false, false);
