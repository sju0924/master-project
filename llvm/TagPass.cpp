#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

// 태그 설정을 위한 LLVM 패스 정의
class StackTagPass : public PassInfoMixin<StackTagPass> {
public:
    // `run` 메서드는 Function 단위로 실행됨
   PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    LLVMContext &context = F.getContext();

        // 'set_tag' 함수에는 이 패스를 적용하지 않도록 제외
        if (F.getName() == "set_tag") {
            return PreservedAnalyses::all();
        }

        bool Modified = false;

        FunctionCallee setTag = M->getOrInsertFunction(
            "set_tag",
            Type::getVoidTy(context),
            PointerType::get(Type::getInt8Ty(context), 0), // 힙 주소 매개변수 타입 (i8*)
            Type::getInt32Ty(context)     // 힙 오브젝트 크기 매개변수 타입 (i64)
        );
        
        // 함수의 모든 기본 블록을 순회
        for (auto &BB : F) {
            for (auto &I : BB) {
                if (AllocaInst *allocInst = dyn_cast<AllocaInst>(&I)) {
                    
                                       
                    Value *Addr = allocInst;
                    auto AllocSize = allocInst->getAllocationSizeInBits(F.getParent()->getDataLayout());
                    if (!AllocSize.hasValue()) {
                        errs() << "Error: Could not determine allocation size.\n";
                        return PreservedAnalyses::none();
                    }
                    Value *Size = Builder.getInt64(AllocSize.getValue() / 8);

                    // 태그 설정 함수 호출을 IR에 삽입
                    IRBuilder<> Builder(I); 
                    Builder.CreateCall(setTag, {Addr, Size});
                    errs() << "Detect function local variable: "<<allocInst->getName()<<" size: "<<Size<<"\n";
                    Modified = true;
                }
            }
        }
        return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());
    }
};

class GlobalVariableTagPass : public PassInfoMixin<GlobalVariableTagPass> {
public:
    // `run` 메서드는 Function 단위로 실행됨
   PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    LLVMContext &context = F.getContext();

    // 'set_tag' 함수에는 이 패스를 적용하지 않도록 제외
    if (F.getName() == "set_tag") {
        return PreservedAnalyses::all();
    }

    bool Modified = false;

    FunctionCallee setTag = M.getOrInsertFunction(
        "set_tag",
        Type::getVoidTy(M.getContext()),
        PointerType::get(Type::getInt8Ty(M.getContext()), 0), // 주소 (i8*)
        Type::getInt8Ty(M.getContext()),                      // 태그 (i8)
        Type::getInt64Ty(M.getContext())                      // 크기 (i64)
    );

    // 모듈의 모든 전역 변수를 순회하며 `set_tag` 호출 삽입
    for (auto &G : M.globals()) {
        if (G.isDeclaration()) continue;

        // 전역 변수 이름과 크기 가져오기
        uint32_t size = M.getDataLayout().getTypeAllocSize(G.getValueType());

        // `set_tag` 호출 삽입 - `main` 함수의 시작 위치
        Function *mainFunc = M.getFunction("main");
        if (mainFunc) {
            IRBuilder<> Builder(&*mainFunc->getEntryBlock().getFirstInsertionPt());

            // `set_tag` 함수 호출 삽입
            Value *Addr = Builder.CreateBitCast(&G, PointerType::get(Type::getInt8Ty(M.getContext()), 0));
            Value *Size = Builder.getInt64(size);
            Builder.CreateCall(setTag, {Addr, Size});
            Modified = true;
        }
    }


    return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());
    
    };
}  

} // namespace

// LLVM 18에 맞춘 패스 등록 코드
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "StackTagPass", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "stack-tag-pass") {
                            FPM.addPass(StackTagPass());
                            return true;
                        }
                        return false;
                    });
            }};
}