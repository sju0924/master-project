#include "TagPass.h"

//todo: 언제 태그 비교 함수를 집어넣느냐(중요)

using namespace llvm;

namespace {

// 태그 설정을 위한 LLVM 패스 정의

// `run` 메서드는 Function 단위로 실행됨
PreservedAnalyses StackTagPass::run(Function &F, FunctionAnalysisManager &AM) {
    LLVMContext &context = F.getContext();
    Module *M = F.getParent();

    // 'set_tag' 함수에는 이 패스를 적용하지 않도록 제외
    if (F.getName() == "set_tag") {
    return PreservedAnalyses::all();
    }

    bool Modified = false;

    FunctionCallee setTag = M->getOrInsertFunction(
        "set_tag",
        Type::getVoidTy(context),
        PointerType::get(Type::getInt8Ty(context), 0), // 힙 주소 매개변수 타입 (i8*)
        Type::getInt32Ty(context)     // 힙 오브젝트 크기 매개변수 타입 (i32)
    );

    // 함수의 모든 기본 블록을 순회
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (AllocaInst *allocInst = dyn_cast<AllocaInst>(&I)) {         
                // 태그 설정 함수 호출을 IR에 삽입
                IRBuilder<> Builder(&I); 
                Value *Addr = Builder.CreateBitCast(allocInst, PointerType::get(Type::getInt8Ty(allocInst->getContext()), 0));
                Value *AllocSize = ConstantInt::get(Type::getInt64Ty(allocInst->getContext()), ((allocInst->getAllocationSizeInBits(F.getParent()->getDataLayout()))->getFixedValue()) / 8);
                Builder.CreateCall(setTag, {Addr, AllocSize});
                errs() << "Detect function local variable: "<<allocInst->getName()<<" size: "<<AllocSize<<"\n";
                Modified = true;
            }
        }
    }
    return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());


};

    // `run` 메서드는 Function 단위로 실행됨
PreservedAnalyses GlobalVariableTagPass::run(Function &F, FunctionAnalysisManager &AM) {
    LLVMContext &context = F.getContext();
    Module *M = F.getParent();

    // 'set_tag' 함수에는 이 패스를 적용하지 않도록 제외
    if (F.getName() == "set_tag") {
        return PreservedAnalyses::all();
    }

    bool Modified = false;

    FunctionCallee setTag = M->getOrInsertFunction(
        "set_tag",
        Type::getVoidTy(M->getContext()),
        PointerType::get(Type::getInt8Ty(M->getContext()), 0), // 주소 (i8*)
        Type::getInt8Ty(M->getContext()),                      // 태그 (i8)
        Type::getInt64Ty(M->getContext())                      // 크기 (i64)
    );

     // 초기화 함수 생성
    Function *initFunc = Function::Create(
        FunctionType::get(Type::getVoidTy(context), false),
        GlobalValue::InternalLinkage,
        "__global_var_init",
        M
    );
    BasicBlock *entry = BasicBlock::Create(context, "entry", initFunc);
    IRBuilder<> Builder(entry);


    // 모듈의 모든 전역 변수를 순회하며 `set_tag` 호출 삽입
    for (auto &G : M->globals()) {
        if (G.isDeclaration()) continue;

        // 전역 변수 이름과 크기 가져오기
        uint32_t size = M->getDataLayout().getTypeAllocSize(G.getValueType());
        
        // `set_tag` 함수 호출 삽입
        Value *Addr = Builder.CreateBitCast(&G, PointerType::get(Type::getInt8Ty(M->getContext()), 0));
        Value *Size = Builder.getInt32(size);
        Builder.CreateCall(setTag, {Addr, Size});

        errs() << "Tagged global variable: " << G.getName() << " Addr: " << &G << " Size: " << size << "\n";
        Modified = true;
        
    }
    Builder.CreateRetVoid();

    // 초기화 함수를 `llvm.global_ctors`에 추가하여 프로그램 시작 시 호출되도록 설정
    appendToGlobalCtors(*M, initFunc, 0);

    return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());
    
};


} // namespace

// LLVM 18에 맞춘 패스 등록 코드
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "TagPasses", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "stack-tag-pass") {
                            FPM.addPass(StackTagPass());
                            return true;
                        }
                        else if (Name == "global-variable-tag-pass") {
                            FPM.addPass(GlobalVariableTagPass());
                            return true;
                        }
                        return false;
                    });
            }};
}