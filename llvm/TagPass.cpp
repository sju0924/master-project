#include "TagPass.h"

//todo: 언제 태그 비교 함수를 집어넣느냐(중요)

using namespace llvm;

namespace {

    
bool isUserDefinedStruct(StructType *structType) {
    return structType->hasName() && structType->getName().startswith("struct.");
}

void insertSetStructTagsCall(Instruction *inst, StructType *structType,  Value* addr, Module *M, LLVMContext &Context) {
    // 구조체의 이름을 통해 인덱스를 찾기
    std::string structName = structType->getName().str();
    auto indexIt = StructMetadataIndexMap.find(structName);
    if (indexIt == StructMetadataIndexMap.end()) {
        errs() << "No metadata index found for struct type: " << structName << "\n";
        return;
    }
    uint32_t structIndex = indexIt->second;

    IRBuilder<> builder(inst->getNextNode());

    // `set_struct_tags` 함수 선언 가져오기 또는 생성
    FunctionCallee setTagFunc = M->getOrInsertFunction(
        "set_struct_tags", Type::getVoidTy(Context),
        Type::getInt8PtrTy(Context),   // 구조체 주소 포인터
        Type::getInt32Ty(Context)      // 구조체 메타데이터 인덱스
    );

    // 두 번째 인자로 메타데이터 인덱스 전달
    Value *indexValue = builder.getInt32(structIndex);

    // `set_struct_tags` 호출 삽입
    builder.CreateCall(setTagFunc, {addr, indexValue});
}
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

    // `set_struct_tags` 함수의 선언 가져오기 또는 삽입
    FunctionCallee setStructTagFunc = M->getOrInsertFunction(
        "set_struct_tags", Type::getVoidTy(Context),
        Type::getInt8PtrTy(Context),   // 구조체 주소 (void*)
        Type::getInt32Ty(Context)      // 구조체 메타데이터 인덱스 (uint32_t)
    );

    // 함수의 모든 기본 블록을 순회
    for (auto &BB : F) {
        for (auto &I : BB) {
            if (AllocaInst *allocInst = dyn_cast<AllocaInst>(&I)) {         
                // 태그 설정 함수 호출을 IR에 삽입
                IRBuilder<> Builder(&I); 
                Value *Addr = Builder.CreateBitCast(allocInst, PointerType::get(Type::getInt8Ty(allocInst->getContext()), 0));
                Value *AllocSize = ConstantInt::get(Type::getInt64Ty(allocInst->getContext()), ((allocInst->getAllocationSizeInBits(F.getParent()->getDataLayout()))->getFixedValue()) / 8);


                // 구조체일 경우 각 멤버별로 태그 할당
                if (isUserDefinedStruct(dyn_cast<StructType>(allocaInst->getAllocatedType()))) {                    
                    insertSetStructTagsCall(&I, structType, Addr, &M, Context);
                }
                else{
                    Builder.CreateCall(setTag, {Addr, AllocSize});
                }
                    
                errs() << "Detect function local variable: "<<allocInst->getName()<<" size: "<<AllocSize<<"\n";
                Modified = true;
            }
        }
    }
    return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());


};

    // `run` 메서드는 Function 단위로 실행됨
PreservedAnalyses GlobalVariableTagPass::run(Module &M, ModuleAnalysisManager &AM) {
    LLVMContext &context = M.getContext();

    bool Modified = false;

    FunctionCallee setTag = M.getOrInsertFunction(
        "set_tag",
        Type::getVoidTy(M.getContext()),
        PointerType::get(Type::getInt8Ty(M.getContext()), 0), // 주소 (i8*)
        Type::getInt32Ty(M.getContext())                      // 크기 (i32)
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
    for (auto &G : M.globals()) {
        if (G.isDeclaration()) continue;

        // .str 및 .rodata 섹션 제외 -> 사용자 정의 전역 변수일 가능성이 높음
         if (G.getLinkage() != GlobalValue::InternalLinkage &&
        G.getLinkage() != GlobalValue::PrivateLinkage){
        
            // 전역 변수 이름과 크기 가져오기
            uint32_t size = M.getDataLayout().getTypeAllocSize(G.getValueType());
            
            // `set_tag` 함수 호출 삽입
            Value *Addr = Builder.CreateBitCast(&G, PointerType::get(Type::getInt8Ty(context), 0));
            Value *Size = Builder.getInt32(size);
            
            // 구조체일 경우 각 멤버별로 태그 할당
            if (isUserDefinedStruct(dyn_cast<StructType>(allocaInst->getAllocatedType()))) {                    
                insertSetStructTagsCall(&I, structType, Addr, &M, Context);
            }
            else{
                Builder.CreateCall(setTag, {Addr, AllocSize});
            }
                

            errs() << "Tagged global variable: " << G.getName() << " Addr: " << &G << " Size: " << size << "\n";
            Modified = true;
        }   
        
        
    }
    Builder.CreateRetVoid();

    // 초기화 함수를 `llvm.global_ctors`에 추가하여 프로그램 시작 시 호출되도록 설정
    appendToGlobalCtors(M, initFunc, 0);

    return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());
    
};



// Function을 받아서 분석하는 run 메서드
PreservedAnalyses PointerArithmeticPass::run(Function &F, FunctionAnalysisManager &AM) {
    // LoopInfo를 가져오기 위해 FunctionAnalysisManager에서 분석 정보 획득
    auto &LI = AM.getResult<LoopAnalysis>(F);
    Module *M = F.getParent();
    IRBuilder<> Builder(F.getContext());
    bool Modified = false;

    // 외부 compare_tag 함수 정의 (void* 형식의 두 인자를 받음)
    FunctionCallee CompareTagFunc = M->getOrInsertFunction(
        "compare_tag", 
        FunctionType::get(Type::getInt8Ty(F.getContext()), 
                            {PointerType::get(Type::getInt8Ty(M->getContext()), 0), 
                            PointerType::get(Type::getInt8Ty(M->getContext()), 0)}, 
                            false)
    );

    for (auto &BB : F) {
        // 현재 블록이 반복문에 포함되어 있는지 확인
        Loop *L = LI.getLoopFor(&BB);
        
        if (L) {
            // 반복문 내에서 첫 번째 및 마지막 접근 요소를 추적
            Value *firstAccess = nullptr;
            Value *lastAccess = nullptr;

            for (auto &I : BB) {
                if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                    // 첫 번째 요소 접근 시 포인터 저장
                    if (!firstAccess) {
                        firstAccess = GEP;
                        errs() << "First element in loop detected: " << *GEP << "\n";
                    }
                    // 반복문 내 마지막 요소 접근을 계속 업데이트
                    lastAccess = GEP;
                }
            }
        

            // 반복문이 끝난 후 첫 번째와 마지막 요소에 대해 태그 비교 수행
            if (firstAccess && lastAccess) {
                Builder.SetInsertPoint(BB.getTerminator());  // 블록의 마지막에 삽입

                // 첫 번째와 마지막 포인터를 i8*로 캐스팅하여 compare_tag 호출
                auto *CastFirst = Builder.CreateBitCast(firstAccess, PointerType::get(Type::getInt8Ty(M->getContext()), 0));
                auto *CastLast = Builder.CreateBitCast(lastAccess, PointerType::get(Type::getInt8Ty(M->getContext()), 0));
                Builder.CreateCall(CompareTagFunc, {CastFirst, CastLast});
                
                Modified = true;
            }
        }
        else {
            // 반복문 외부의 모든 GEP 감지
            for (auto &I : BB) {
                if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
                    Builder.SetInsertPoint(GEP->getNextNode());
                    Value *Addr1 = GEP->getPointerOperand();
                    Value *Addr2 = GEP;
                    auto *CastAddr1 = Builder.CreateBitCast(Addr1, PointerType::get(Type::getInt8Ty(M->getContext()), 0));
                    auto *CastAddr2 = Builder.CreateBitCast(Addr2, PointerType::get(Type::getInt8Ty(M->getContext()), 0));
                    Builder.CreateCall(CompareTagFunc, {CastAddr1, CastAddr2});

                    errs() << "GEP outside loop detected: " << *GEP << "\n";
                    Modified = true;;
                }
            }
        }
    }
    
    return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());
}

PreservedAnalyses StructMetadataPass::run(Module &M, ModuleAnalysisManager &AM)  {
    const DataLayout &DL = M.getDataLayout();
    LLVMContext &Context = M.getContext();
    bool Modified = false;

    // 전역 배열로 저장할 메타데이터들
    std::vector<Constant *> offsetsArray;
    std::vector<Constant *> sizesArray;
    std::vector<Constant *> countsArray;

    // 각 구조체별 인덱스 저장
    std::map<std::string, uint32_t> StructMetadataIndexMap;

    uint32_t numMembers = 0;
    // 모든 구조체 타입에 대해 메타데이터 수집
    for (StructType *structType : M.getIdentifiedStructTypes()) {
        if (!isUserDefinedStruct(structType))
            continue;

        // 고유 인덱스를 할당하여 매핑 테이블에 저장
        std::string structName = structType->getName().str();
        StructMetadataIndexMap[structName] = CurrentIndex++;

        std::vector<Constant *> offsets, sizes;
        numMembers = collectStructMetadata(structType, DL, Context, offsets, sizes);

        countsArray.push_back(ConstantInt::get(Type::getInt32Ty(Context), numMembers));
        offsetsArray.push_back(offsets);
        sizesArray.push_back(sizes);
    }

    // 전역 배열로 모듈에 추가
    if(numMembers){
        for (size_t i = 0; i < offsetsArray.size(); ++i) {
            ArrayType *innerOffsetsArrayType = ArrayType::get(Type::getInt64Ty(Context), offsetsArray[i].size());
            ArrayType *innerSizesArrayType = ArrayType::get(Type::getInt64Ty(Context), sizesArray[i].size());

            offsetsGlobalArray.push_back(ConstantArray::get(innerOffsetsArrayType, offsetsArray[i]));
            sizesGlobalArray.push_back(ConstantArray::get(innerSizesArrayType, sizesArray[i]));
        }

        new GlobalVariable(M, ArrayType::get(offsetsArrayType, offsetsGlobalArray.size()), true, GlobalValue::ExternalLinkage, ConstantArray::get(offsetsGlobalArray), "struct_member_offsets");
        new GlobalVariable(M, ArrayType::get(sizesArrayType, sizesGlobalArray.size()), true, GlobalValue::ExternalLinkage, ConstantArray::get(sizesGlobalArray), "struct_member_sizes");
        new GlobalVariable(M, ArrayType::get(Type::getInt64Ty(Context), countsArray.size()), true, GlobalValue::ExternalLinkage, ConstantArray::get(countsArray), "struct_member_counts");

   }


    return (Modified ? PreservedAnalyses::none() : PreservedAnalyses::all());
};


uint32_t StructMetadataPass::collectStructMetadata(StructType *structType, const DataLayout &DL, LLVMContext &Context,
                            std::vector<Constant *> &offsetsArray, std::vector<Constant *> &sizesArray) {

    uint32_t elementNum = structType->getNumElements();
    for (unsigned i = 0; i < elementNum; ++i) {
        uint64_t memberOffset = DL.getStructLayout(structType)->getElementOffset(i);
        uint64_t memberSize = DL.getTypeAllocSize(structType->getElementType(i));

        offsetsArray.push_back(ConstantInt::get(Type::getInt32Ty(Context), memberOffset));
        sizesArray.push_back(ConstantInt::get(Type::getInt32Ty(Context), memberSize));
    }

    return elementNum;
}


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
                        else if (Name == "arithmetic-pointer-tag-pass") {
                            FPM.addPass(PointerArithmeticPass());
                            return true;
                        }
                        return false;
                    });
                
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, ModulePassManager &MPM,
                       ArrayRef<PassBuilder::PipelineElement>) {
                        if (Name == "global-variable-tag-pass") {
                            MPM.addPass(GlobalVariableTagPass());
                            return true;
                        }
                        return false;
                    });
            }};
}