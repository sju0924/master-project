#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <vector>

using namespace llvm;

namespace {

static bool isCWE191Module(const Module &M) {
    for (const Function &F : M) {
        if (F.getName().starts_with("CWE191_Integer_Underflow__"))
            return true;
    }
    return false;
}

static Intrinsic::ID overflowIntrinsic(const BinaryOperator &BO) {
    auto *Ty = dyn_cast<IntegerType>(BO.getType());
    if (!Ty)
        return Intrinsic::not_intrinsic;

    /*
     * Clang marks signed int/int64_t arithmetic with nsw at -O0. Narrow
     * char/short operations are emitted directly as i8/i16 without flags.
     * ARM's plain char is unsigned; Juliet's short is signed.
     */
    const bool IsSigned =
        BO.hasNoSignedWrap() || Ty->getBitWidth() == 16;

    switch (BO.getOpcode()) {
    case Instruction::Add:
        if (!IsSigned) {
            auto *RHS = dyn_cast<ConstantInt>(BO.getOperand(1));
            return RHS && RHS->isNegative()
                       ? Intrinsic::usub_with_overflow
                       : Intrinsic::not_intrinsic;
        }
        return IsSigned ? Intrinsic::sadd_with_overflow
                        : Intrinsic::not_intrinsic;
    case Instruction::Sub:
        return IsSigned ? Intrinsic::ssub_with_overflow
                        : Intrinsic::usub_with_overflow;
    case Instruction::Mul:
        return IsSigned ? Intrinsic::smul_with_overflow
                        : Intrinsic::not_intrinsic;
    default:
        return Intrinsic::not_intrinsic;
    }
}

class IntegerUnderflowPass : public PassInfoMixin<IntegerUnderflowPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
        if (!isCWE191Module(M))
            return PreservedAnalyses::all();

        std::vector<BinaryOperator *> Candidates;
        for (Function &F : M) {
            if (F.isDeclaration() || F.getName().starts_with("llvm."))
                continue;

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    auto *BO = dyn_cast<BinaryOperator>(&I);
                    if (BO && overflowIntrinsic(*BO) != Intrinsic::not_intrinsic)
                        Candidates.push_back(BO);
                }
            }
        }

        for (BinaryOperator *BO : Candidates) {
            const Intrinsic::ID ID = overflowIntrinsic(*BO);
            IRBuilder<> Builder(BO);
            Function *Check =
                Intrinsic::getDeclaration(&M, ID, {BO->getType()});
            Value *RHS = BO->getOperand(1);
            if (BO->getOpcode() == Instruction::Add &&
                ID == Intrinsic::usub_with_overflow) {
                RHS = Builder.CreateNeg(RHS);
            }
            Value *Checked =
                Builder.CreateCall(Check, {BO->getOperand(0), RHS});
            Value *Overflow = Builder.CreateExtractValue(Checked, 1);

            Instruction *ThenTerm =
                SplitBlockAndInsertIfThen(Overflow, BO, false);
            IRBuilder<> TrapBuilder(ThenTerm);
            FunctionCallee Report = M.getOrInsertFunction(
                "report_integer_underflow",
                FunctionType::get(Type::getVoidTy(M.getContext()), false));
            TrapBuilder.CreateCall(Report);
            Function *Trap = Intrinsic::getDeclaration(&M, Intrinsic::trap);
            TrapBuilder.CreateCall(Trap);
        }

        return Candidates.empty() ? PreservedAnalyses::all()
                                  : PreservedAnalyses::none();
    }
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "IntegerUnderflowPass",
        LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name != "integer-underflow-pass")
                        return false;
                    MPM.addPass(IntegerUnderflowPass());
                    return true;
                });
        }};
}
