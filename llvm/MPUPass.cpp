#include "MPUPass.h"

PreservedAnalyses StackMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  

    
    if (F.getName() == "configure_mpu_redzone_for_call" || F.getName() == "configure_mpu_redzone_for_return" || F.getName()=="__global_var_init" || F.getName()=="set_tag" || F.getName()=="my_malloc" || F.getName()=="my_free" ) {
        return PreservedAnalyses::none();
    }
    errs() << "Analyzing function: " << F.getName() << "\n";
    // 함수 실행 시 Stack Redzone 설정
    Module *M = F.getParent();
    
    BasicBlock &EntryBlock = F.getEntryBlock();
    LLVMContext &context = F.getContext();

    

    // configure_mpu_redzone_for_call 함수 선언을 찾거나 생성
    FunctionCallee configureMPURedzoneForCall = M->getOrInsertFunction(
        "configure_mpu_redzone_for_call",
        FunctionType::get(
            Type::getVoidTy(context),                    // 반환 타입 (void)
            {Type::getInt32Ty(context), Type::getInt32Ty(context)},  // 매개변수 타입들 (uint32_t, uint32_t)
            false                                        // 가변 인자 여부 (false)
        )
    );

    // RSP 조정을 위한 어셈블리 코드 삽입
    // Inline assembly to adjust rsp by 64 bytes (redzone)
    InlineAsm *SubRSP = InlineAsm::get(
        FunctionType::get(Type::getVoidTy(context), false),
        "sub sp, 72", "", true);

    InlineAsm *AddRSP = InlineAsm::get(
    FunctionType::get(Type::getVoidTy(context), false),
    "add sp, 72", "", true);

    //메모리 배리어
    InlineAsm *MemoryBarrier = InlineAsm::get(
    FunctionType::get(Type::getVoidTy(context), false),
    "dmb sy", "", true // 명령어 강제 적용
);
    // `sp` 값을 불러오는 인라인 ASM
    InlineAsm *AsmSp = InlineAsm::get(
        FunctionType::get(Type::getInt32Ty(context), false), 
        "mov $0, sp",                                     
        "=r",                                             
        true                                               
    );

    // `r7` 값을 불러오는 인라인 ASM
    InlineAsm *AsmR7 = InlineAsm::get(
        FunctionType::get(Type::getInt32Ty(context), false),  // 반환 타입: int32
        "mov $0, r7",                                        // 어셈블리 코드
        "=r",                                                // 출력 제약 조건
        true                                                 // 읽기 전용 여부
    );

    // 함수의 시작 부분에 configure_mpu_redzone_for_call 호출을 삽입
    IRBuilder<> builder(&EntryBlock.front());
    Value *SpVal = builder.CreateCall(AsmSp);
    Value *R7Val = builder.CreateCall(AsmR7);
    builder.CreateCall(configureMPURedzoneForCall, {SpVal, R7Val});


    for (auto &BB : F) {
        for (auto I = BB.begin(); I != BB.end(); ++I) {
            // CallInst를 통해 함수 호출 감지
            if (CallInst *CI = dyn_cast<CallInst>(&*I)) {
                Function *calledFunc = CI->getCalledFunction();
                if (calledFunc) {
                    if (calledFunc->getName() == "configure_mpu_redzone_for_call" ||
                        calledFunc->getName() == "configure_mpu_redzone_for_return" ||
                        (calledFunc->getName().find("llvm.") == 0 && calledFunc->getName().find("mem") == std::string::npos) ) {
                        continue;
                    }

                    errs() << "  Function call detected: " 
                           << F.getName() << " calls " << calledFunc->getName() << "\n";
                } 
                else {
                    errs() << "  Indirect function call detected in function: " 
                           << F.getName() << "\n";

                     if (isa<InlineAsm>(CI->getCalledOperand())) {
                        errs() << "  InlineAsm call detected\n";
                        continue;
                    } else {
                        errs() << "  Indirect function call detected\n";
                    }
                }  

                
                
                // 함수 호출 전: "sub rsp, 64" 삽입
                IRBuilder<> BuilderBefore(CI);
                BuilderBefore.SetInsertPoint(CI);
                BuilderBefore.CreateCall(SubRSP);

                // 함수 호출 후: "add rsp, 64" 삽입
                ++I; // Move iterator to the next instruction
                if (I != BB.end()) { // Check if iterator is still valid
                    IRBuilder<> BuilderAfter(&*I); 
                    if (calledFunc && calledFunc->getName() != "configure_mpu_redzone_for_heap_access" && calledFunc->getName()!= "configure_mpu_redzone_for_global" &&calledFunc->getName() != "configure_mpu_for_poison"&&calledFunc->getName()!= "set_tag"&&calledFunc->getName() != "compare_tag" &&  calledFunc->getName() != "HAL_MPU_Disable"&&  calledFunc->getName() != "HAL_MPU_Enable"&&  calledFunc->getName() != "HAL_GetTick"  ){
                        BuilderAfter.CreateCall(configureMPURedzoneForCall, {SpVal, R7Val});   
                    }                         
                    
                    BuilderAfter.CreateCall(AddRSP);             
                }
                else{
                    IRBuilder<> BuilderRedzone(CI->getNextNode());                    
                    if (calledFunc && calledFunc->getName() != "configure_mpu_redzone_for_heap_access" && calledFunc->getName() != "configure_mpu_redzone_for_global" && calledFunc->getName() != "configure_mpu_for_poison"&&calledFunc->getName() != "set_tag"&&calledFunc->getName() != "compare_tag"&&  calledFunc->getName() != "HAL_MPU_Disable"&&  calledFunc->getName() != "HAL_MPU_Enable"&&  calledFunc->getName() != "HAL_GetTick" ){
                        BuilderRedzone.CreateCall(configureMPURedzoneForCall, {SpVal, R7Val});   
                    }
                    BuilderRedzone.CreateCall(AddRSP);         
                }
                
                --I; 

                              
                // Restore iterator to point at the original call
                
            }          
        }
        // Return analysis: ReturnInst를 통해 함수 리턴 감지
        if (ReturnInst *Ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
            IRBuilder<> RetBuilder(Ret);

             FunctionCallee configureMPURedzoneForReturn = M->getOrInsertFunction(
                "configure_mpu_redzone_for_return",
                Type::getVoidTy(context)  // 반환 타입 (void)
            );

            // 함수 리턴 후 
            RetBuilder.SetInsertPoint(Ret);  // Ret 위치에 삽입 지점 설정
            RetBuilder.CreateCall(SubRSP);
            RetBuilder.CreateCall(configureMPURedzoneForReturn);
            RetBuilder.CreateCall(AddRSP);

 
            errs() << "  Return detected in function: " 
                    << F.getName() << "\n";
        }
    }

    // 모든 분석 정보를 보존하도록 설정
    return PreservedAnalyses::all();
}

PreservedAnalyses HeapMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    // 함수 내 모든 명령어를 순회하며 힙 접근 검사
    Module *M = F.getParent();
    bool modified = false;
    LLVMContext &context = F.getContext();
    const DataLayout &dataLayout = F.getParent()->getDataLayout();
    IRBuilder<> Builder(F.getContext());

     // configure_mpu_redzone_for_heap_access 함수 선언을 가져오거나 생성
    FunctionCallee configureMPURedzoneForHeapAccess = M->getOrInsertFunction(
        "configure_mpu_redzone_for_heap_access",
        Type::getVoidTy(context),
        PointerType::get(Type::getInt8Ty(context), 0)
    );


    for (auto &BB : F) {
            for (auto &I : BB) {
                // 힙 메모리 할당 함수 (malloc, calloc 등) 감지
                if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                    if (Function *calledFunc = CI->getCalledFunction()) {
                        StringRef funcName = calledFunc->getName();
                        if (funcName == "my_malloc" || funcName == "calloc" || funcName == "realloc") {
                            // 힙 오브젝트 포인터와 크기를 저장
                            if (ConstantInt *size = dyn_cast<ConstantInt>(CI->getArgOperand(0))) {
                                uint64_t allocSize = size->getZExtValue();
                                errs() << "Heap object Allocation Detected: "<< CI<<"\n";
                                ///unsigned typeSize = dataLayout.getTypeAllocSize(CI->getType());
                                heapObjects[CI] = allocSize; // * typeSize;  // 시작 주소와 크기 저장
                                
                            }
                            continue;
                        }
                    }
                }

                // 메모리 해제 함수 (free) 호출 감지
                if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                    if (Function *calledFunc = CI->getCalledFunction()) {
                        if (calledFunc->getName() == "my_free") {
                            Value *freedPtr = CI->getArgOperand(0);
                            removeHeapObject(freedPtr);
                        }
                    }
                }
                

                // load/store 명령어로 힙 오브젝트 접근 감지
                if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
                    if(checkHeapAccessChanged(LI->getPointerOperand(), &I)){
                        Builder.SetInsertPoint(LI);
                        Builder.CreateCall(configureMPURedzoneForHeapAccess, lastHeapObject);
                        modified = true;
                    }
                } else if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
                    if(checkHeapAccessChanged(SI->getPointerOperand(), &I)){
                        Builder.SetInsertPoint(SI);
                        Builder.CreateCall(configureMPURedzoneForHeapAccess, lastHeapObject);
                        modified = true;
                    }
                }
            }
        }

    // my_malloc과 my_free 함수 선언 또는 가져오기
    Type *Int8PtrTy = PointerType::get(Type::getInt8Ty(context), 0);  // i8* 타입 생성
    FunctionCallee MyMallocFunc = M->getOrInsertFunction(
        "my_malloc",
        FunctionType::get(Int8PtrTy, { Type::getInt32Ty(context) }, false) // i32 타입으로 설정
    );

    FunctionCallee MyFreeFunc = M->getOrInsertFunction(
        "my_free",
        FunctionType::get(Type::getVoidTy(context), { Int8PtrTy }, false)
    );
    // 모든 분석 정보를 보존한다고 설정
     return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}


void HeapMPUPass::removeHeapObject(Value *freedPtr) {
    if (heapObjects.count(freedPtr) > 0) {
        heapObjects.erase(freedPtr);
        errs() << "Heap object freed and removed from tracking.\n";
    } else {
        errs() << "Attempt to free untracked memory.\n";
    }
}

Value* HeapMPUPass::_isHeapObject( Instruction *I, Value *currentPtr) {
    Value *ptr = currentPtr;
    if(!ptr){
        return nullptr;
    }
    while (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {
            ptr = gep->getPointerOperand(); // GEP의 기본 포인터를 재귀적으로 추적
        }

    // 힙 포인터 집합에 포함되어 있는지 확인
    if (heapObjects.count(ptr)) {
        return ptr;
    }

    return nullptr;
}


bool HeapMPUPass::checkHeapAccessChanged(Value *currentPtr, Instruction *I) {

   if (Value *heapObject = _isHeapObject(I, currentPtr)) {
                // lastHeapObject가 유효하지 않거나 해제된 경우 초기화
                if (lastHeapObject && heapObjects.find(lastHeapObject) == heapObjects.end()) {
                    lastHeapObject = nullptr;
                    errs() << "Initialized lastHeapObject \n";
                }
                
                if (lastHeapObject != heapObject) {
                    errs() << "Accessing a different heap object at: " << *I << "\n";
                    lastHeapObject = heapObject;
                    return true;
                } else {
                    errs() << "Accessing the same heap object at: " << *I << "\n";
                    return false;
                }
                
            }
    return false;
}


PreservedAnalyses GlobalVariableMPUPass::run(Module &M, ModuleAnalysisManager &AM) {
    
    /////////////////////////////
    // 전역 변수에 REDZONE 삽입 //
    /////////////////////////////

    const int GLOBAL_REDZONE_SIZE = 32;
    LLVMContext &context = M.getContext();
    Type *int8Ty = Type::getInt8Ty(context);
    ArrayType *redzoneType = ArrayType::get(int8Ty, GLOBAL_REDZONE_SIZE); // 32바이트 레드존 타입
    const DataLayout &dataLayout = M.getDataLayout();
    
    // 전역 변수 교체 및 MPU 보호 함수 호출 삽입
    std::vector<GlobalVariable*> globalsToReplace;

     // 레드존을 추가할 전역 변수 식별
    for (GlobalVariable &GV : M.globals()) {
        globalsToReplace.push_back(&GV);
    }

    // 전역 변수 교체 및 MPU 보호 함수 호출 삽입
    for (GlobalVariable *GV : globalsToReplace) {
        Type *originalType = GV->getValueType();
        uint64_t globalSize = dataLayout.getTypeAllocSize(originalType); // 전역 변수 크기 계산

        if (GV && (GV->getName().str().find("_with_redzone") != std::string::npos ||GV->getName().str().find(".") != std::string::npos)){
            continue;
        }

        if (GV && (GV->getName().str() == "struct_member_offsets" ||GV->getName().str() == "struct_member_sizes" ||GV->getName().str() == "struct_member_counts")){
            continue;
        }

        // 전역 변수 + 레드존을 포함하는 새로운 구조체 타입 생성
        // 필요한 패딩을 계산
        uint64_t paddingSize = (32 - (globalSize % 32)) % 32;

        // 패딩 크기만큼의 바이트 배열 타입을 정의 (0일 경우 빈 배열이 됩니다)
        ArrayType *paddingType = ArrayType::get(Type::getInt8Ty(originalType->getContext()), paddingSize);

        // 전역 변수 + 레드존 + 패딩을 포함하는 새로운 구조체 타입 생성
        StructType *structWithRedzoneType;
        if (paddingSize > 0) {
            structWithRedzoneType = StructType::create(
                {redzoneType, originalType, paddingType, redzoneType}, // [앞 레드존, 전역 변수, 패딩, 뒤 레드존]
                GV->getName().str() + "_with_redzone"
            );
        } else {
            structWithRedzoneType = StructType::create(
                {redzoneType, originalType, redzoneType}, // 패딩이 필요 없을 때 [앞 레드존, 전역 변수, 뒤 레드존]
                GV->getName().str() + "_with_redzone"
            );
        }

        // 새로운 전역 변수 생성
        GlobalVariable *newGV = new GlobalVariable(
            M,
            structWithRedzoneType,
            GV->isConstant(),
            GV->getLinkage(),
            nullptr,
            GV->getName() + "_with_redzone"
        );

        // 기존 초기화 값 설정
        Constant *zeroInit = ConstantAggregateZero::get(redzoneType);
        Constant *originalInit = GV->hasInitializer() ? GV->getInitializer() : ConstantAggregateZero::get(originalType);

        // 패딩 초기화 값 설정 (필요한 경우)
        Constant *paddingInit = ConstantAggregateZero::get(paddingType);

        // 초기화 값에 패딩을 포함하여 초기화 생성
        Constant *initializer;
        if (paddingSize > 0) {
            initializer = ConstantStruct::get(
                structWithRedzoneType, {zeroInit, originalInit, paddingInit, zeroInit}
            );
        } else {
            initializer = ConstantStruct::get(
                structWithRedzoneType, {zeroInit, originalInit, zeroInit}
            );
        }

        newGV->setInitializer(initializer);
        newGV->setAlignment(Align(32)); // 32바이트 정렬 설정

        // 기존 전역 변수 참조 교체
        GV->replaceAllUsesWith(ConstantExpr::getInBoundsGetElementPtr(
            structWithRedzoneType, newGV,
            ArrayRef<Constant*>{ConstantInt::get(Type::getInt32Ty(context), 0),
                ConstantInt::get(Type::getInt32Ty(context), 1)}
        ));

        GV->eraseFromParent(); // 기존 전역 변수 삭제
    }

    // configure_mpu_redzone_for_global 함수 호출 삽입
    FunctionCallee configureMPURedzoneForGlobal = M.getOrInsertFunction(
        "configure_mpu_redzone_for_global",
        Type::getVoidTy(context),
        PointerType::get(Type::getInt8Ty(context), 0), // 전역 변수 주소 매개변수 타입 (i8*)
        Type::getInt64Ty(context)     // 전역 변수 크기 매개변수 타입 (i64)
    );

    ////////////////////////////
    // 전역 변수 접근 변경 탐지 //
    ////////////////////////////
    for (auto &GV : M.globals()) {
        globalVars.insert(&GV);
        errs() << "Tracking global variable: " << GV.getName() << "\n";
    }

    Value *globalPointer = nullptr;
    Value *globalBodyPtr = nullptr;
    Value *runtimeBodyPtr =nullptr;
    Value *runtimeBodySize = nullptr;
    for (auto &F : M.functions()) {
        lastGlobalVariable = nullptr;
        for (auto &BB : F) {                
                for (auto &I : BB) {
                    // load 명령어에서 전역 변수 접근 탐지
                    if (auto *LI = dyn_cast<LoadInst>(&I)) {
                        if(GlobalVariable* GV = CheckGlobalVariableAccessChanged(LI->getPointerOperand(), &I)){

                            // 크기: 레드존을 포함한 전체 구조체의 크기 계산
                            uint64_t structSize = dataLayout.getTypeAllocSize(GV->getValueType());;

                            // 전역 변수 본체의 포인터 추적
                            if (auto *structType = dyn_cast<StructType>(GV->getValueType())) {
                                if (structType->getNumElements() == 3) { // 앞 레드존, 전역 변수, 뒤 레드존
                                    // 구조체의 두 번째 필드를 통해 전역 변수 본체에 접근
                                    globalBodyPtr = ConstantExpr::getInBoundsGetElementPtr(
                                        structType, GV,
                                        ArrayRef<Constant*>{ConstantInt::get(Type::getInt32Ty(GV->getContext()), 0),  // 구조체 기본 주소
                                        ConstantInt::get(Type::getInt32Ty(GV->getContext()), 1)}  // 두 번째 필드
                                    );                                  
                                }
                                else if(structType->getNumElements() == 4) { // 앞 레드존, 전역 변수, 패딩, 뒤 레드존
                                    // 구조체의 두 번째 필드를 통해 전역 변수 본체에 접근
                                    // 전역 변수 본체의 포인터 추적
                                    globalBodyPtr = ConstantExpr::getInBoundsGetElementPtr(
                                        structType, GV,
                                        ArrayRef<Constant*>{
                                            ConstantInt::get(Type::getInt32Ty(GV->getContext()), 0), // 구조체 기본 주소
                                            ConstantInt::get(Type::getInt32Ty(GV->getContext()), 1)  // 전역 변수 본체의 필드 인덱스 (1)
                                        }
                                    );                               
                                }
                                // 전역 변수 본체의 크기 계산 (레드존 제외)
                                uint64_t globalBodySize = dataLayout.getTypeAllocSize(structType->getElementType(1));

                                // configure_mpu_redzone_for_global 호출 (IRBuilder에서 접근 가능하도록 포인터 준비)
                                IRBuilder<> builder(&I); // LI 위치에 빌더 설정
                                runtimeBodyPtr = builder.CreateBitCast(globalBodyPtr, PointerType::get(Type::getInt8Ty(GV->getContext()), 0));
                                runtimeBodySize = ConstantInt::get(Type::getInt64Ty(GV->getContext()), globalBodySize);
                                builder.CreateCall(configureMPURedzoneForGlobal, {runtimeBodyPtr, runtimeBodySize});
                               // errs() << "Set global variable MPU: " << GV->getName() << "\n";

                                
                            }
                            
                           

                        }
                    }
                    // store 명령어에서 전역 변수 접근 탐지
                    else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                        if(GlobalVariable* GV = CheckGlobalVariableAccessChanged(SI->getPointerOperand(), &I)){

                            // 크기: 레드존을 포함한 전체 구조체의 크기 계산
                            uint64_t structSize = dataLayout.getTypeAllocSize(GV->getValueType());
                           // errs()<<"ValueType: "<<GV->getValueType()<<" GV: "<<GV->getName()<<"\n";

                            // 전역 변수 본체의 포인터 추적
                            if (auto *structType = dyn_cast<StructType>(GV->getValueType())) {
                                if (structType->getNumElements() == 3) { // 앞 레드존, 전역 변수, 뒤 레드존
                                    // 구조체의 두 번째 필드를 통해 전역 변수 본체에 접근
                                    globalBodyPtr = ConstantExpr::getInBoundsGetElementPtr(
                                        structType, GV,
                                        ArrayRef<Constant*>{ConstantInt::get(Type::getInt32Ty(GV->getContext()), 0),  // 구조체 기본 주소
                                        ConstantInt::get(Type::getInt32Ty(GV->getContext()), 1)}  // 두 번째 필드
                                    );                                  
                                }
                                else if(structType->getNumElements() == 4) { // 앞 레드존, 전역 변수, 패딩, 뒤 레드존
                                    // 구조체의 두 번째 필드를 통해 전역 변수 본체에 접근
                                    // 전역 변수 본체의 포인터 추적
                                    globalBodyPtr = ConstantExpr::getInBoundsGetElementPtr(
                                        structType, GV,
                                        ArrayRef<Constant*>{
                                            ConstantInt::get(Type::getInt32Ty(GV->getContext()), 0), // 구조체 기본 주소
                                            ConstantInt::get(Type::getInt32Ty(GV->getContext()), 1)  // 전역 변수 본체의 필드 인덱스 (1)
                                        }
                                    );                               
                                }
                                // 전역 변수 본체의 크기 계산 (레드존 제외)
                                uint64_t globalBodySize = dataLayout.getTypeAllocSize(structType->getElementType(1));

                                // configure_mpu_redzone_for_global 호출 (IRBuilder에서 접근 가능하도록 포인터 준비)
                                IRBuilder<> builder(&I); // LI 위치에 빌더 설정
                                runtimeBodyPtr = builder.CreateBitCast(globalBodyPtr, PointerType::get(Type::getInt8Ty(GV->getContext()), 0));
                                runtimeBodySize = ConstantInt::get(Type::getInt64Ty(GV->getContext()), globalBodySize);
                                builder.CreateCall(configureMPURedzoneForGlobal, {runtimeBodyPtr, runtimeBodySize});
                              //  errs() << "Set global variable MPU: " << GV->getName() << "\n";

                                
                            }
                            
                        }
                    }
                }
            }
        }
    return PreservedAnalyses::all();
}

  
Value* GlobalVariableMPUPass::isGlobalVariable(Value *currentPtr){
    Value *ptr = currentPtr;
    if (!ptr) {
        return nullptr;
    }

    // `GetElementPtrInst`와 `ConstantExpr`를 재귀적으로 추적하여 최종 전역 변수에 도달
    while (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {
        ptr = gep->getPointerOperand();
    }

    // `ConstantExpr`를 통한 `GEP` 추적
    if (auto *constExpr = dyn_cast<ConstantExpr>(ptr)) {
        if (constExpr->getOpcode() == Instruction::GetElementPtr) {
            ptr = constExpr->getOperand(0); // 기본 포인터를 가져옴
        }
    }

    // 최종 추적된 포인터가 전역 변수인지 확인
    if (auto *GV = dyn_cast<GlobalVariable>(ptr)) {
        if (globalVars.count(GV)) {
            return GV;  // 최종 전역 변수 반환
        }
    } 
    return nullptr;
}

// 초기화된 상수가 아닌 전역 변수인지 확인하고 포인터 반환
GlobalVariable* GlobalVariableMPUPass::istInitializedNonConstantGlobalVariable(Value *ptrOperand) {
    if (auto *GV = dyn_cast<GlobalVariable>(ptrOperand)) {
        // 전역 변수가 초기화된 상수가 아닌지 확인
        if (GV->hasInitializer() && !GV->isConstant()) {
            return GV;
        }
    }
    return nullptr;
}


GlobalVariable* GlobalVariableMPUPass::CheckGlobalVariableAccessChanged(Value *currentPtr, Instruction *I){
    if (Value *gv = isGlobalVariable(currentPtr)) {
        if(GlobalVariable* currentGlobalVariable = dyn_cast<GlobalVariable>(gv)){
                    // lastGlobalVariable 유효하지 않거나 해제된 경우 초기화
            if (lastGlobalVariable && globalVars.find(lastGlobalVariable) == globalVars.end()) {
                lastGlobalVariable = nullptr;
            }
           // errs()<<"last GV: "<<lastGlobalVariable<<", cur GV: "<<currentGlobalVariable<<"\n";
            if (lastGlobalVariable != currentGlobalVariable) {
                errs() << "Accessing a different global variable at: " << *I << "\n";
                lastGlobalVariable = currentGlobalVariable;
                return currentGlobalVariable;
            } else {
                errs() << "Accessing the same global object at: " << *I << "\n";
                
            }
        }

        
    }
    
    return nullptr;
}

PreservedAnalyses NullPtrMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    if (F.getName() != "main") {
            return PreservedAnalyses::all();  // main이 아닌 함수는 처리하지 않음
        }

        // 모듈에 접근
        Module *M = F.getParent();

        // 호출할 configure_mpu_for_null_ptr 함수가 모듈에 존재하는지 확인하거나 새로 생성
        FunctionCallee MPUConfigFunc = M->getOrInsertFunction("configure_mpu_for_null_ptr",
                                                              FunctionType::get(Type::getVoidTy(M->getContext()), false));

        // main 함수의 시작 부분에 configure_mpu_for_null_ptr 호출 삽입
        IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());
        Builder.CreateCall(MPUConfigFunc);
        errs() << "Insert null ptr guard at main: " << &*F.getEntryBlock().getFirstInsertionPt() << "\n";

        return PreservedAnalyses::none();
                                      
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
                    } else if (Name == "null-ptr-mpu-pass") {
                        FPM.addPass(NullPtrMPUPass());
                        return true;
                    }
                    return false;
                }
            );
            PB.registerPipelineParsingCallback(
            [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
                if (Name == "global-variable-mpu-pass") {
                    MPM.addPass(GlobalVariableMPUPass());
                    return true;
                }
                return false;
            });
        }};
}