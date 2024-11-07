#include "MPUPass.h"

PreservedAnalyses StackMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  errs() << "Analyzing function: " << F.getName() << "\n";

    
    if (F.getName() == "configure_mpu_redzone_for_call" || F.getName() == "configure_mpu_redzone_for_return") {
        return PreservedAnalyses::none();
    }
    // 함수 실행 시 Stack Redzone 설정
    Module *M = F.getParent();

    BasicBlock &EntryBlock = F.getEntryBlock();
    IRBuilder<> builder(&EntryBlock.front());
    LLVMContext &context = F.getContext();

    // configure_mpu_redzone_for_call 함수 선언을 찾거나 생성
    FunctionCallee configureMPURedzoneForCall = M->getOrInsertFunction(
        "configure_mpu_redzone_for_call",
        Type::getVoidTy(context)  // 반환 타입 (void)
    );
    // 함수의 시작 부분에 configure_mpu_redzone_for_call 호출을 삽입
    builder.CreateCall(configureMPURedzoneForCall);

    // RSP 조정을 위한 어셈블리 코드 삽입
    // Inline assembly to adjust rsp by 64 bytes (redzone)
    InlineAsm *SubRSP = InlineAsm::get(
        FunctionType::get(Type::getVoidTy(context), false),
        "sub rsp, 72", "", true);

    InlineAsm *AddRSP = InlineAsm::get(
        FunctionType::get(Type::getVoidTy(context), false),
        "add rsp, 72", "", true);

    for (auto &BB : F) {
        for (auto I = BB.begin(); I != BB.end(); ++I) {
            // CallInst를 통해 함수 호출 감지
            if (CallInst *CI = dyn_cast<CallInst>(&*I)) {
                if (Function *calledFunc = CI->getCalledFunction()) {
                    if (calledFunc->getName() == "configure_mpu_redzone_for_call" ||
                        calledFunc->getName() == "configure_mpu_redzone_for_return") {
                        continue;
                    }

                    errs() << "  Function call detected: " 
                           << F.getName() << " calls " 
                           << calledFunc->getName() << "\n";
                } else {
                    errs() << "  Indirect function call detected in function: " 
                           << F.getName() << "\n";
                }  
                
                // 함수 호출 전: "sub rsp, 64" 삽입
                IRBuilder<> BuilderBefore(CI);
                BuilderBefore.SetInsertPoint(CI); 
                BuilderBefore.CreateCall(SubRSP);

                // 함수 호출 후: "add rsp, 64" 삽입
                ++I; // Move iterator to the next instruction
                if (I != BB.end()) { // Check if iterator is still valid
                    IRBuilder<> BuilderAfter(&*I);
                    BuilderAfter.CreateCall(AddRSP);
                }
                --I; 

                              
                // Restore iterator to point at the original call
                IRBuilder<> BuilderRedzone(CI->getNextNode());
                BuilderRedzone.CreateCall(configureMPURedzoneForCall);
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
                RetBuilder.CreateCall(configureMPURedzoneForReturn);
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
    // 함수 내 모든 명령어를 순회하며 힙 접근 검사
    Module *M = F.getParent();

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
                        if (funcName == "malloc" || funcName == "calloc" || funcName == "realloc") {
                            // 힙 오브젝트 포인터와 크기를 저장
                            if (ConstantInt *size = dyn_cast<ConstantInt>(CI->getArgOperand(0))) {
                                uint64_t allocSize = size->getZExtValue();
                                ///unsigned typeSize = dataLayout.getTypeAllocSize(CI->getType());
                                heapObjects[CI] = allocSize; // * typeSize;  // 시작 주소와 크기 저장
                                lastHeapObject = CI;
                                errs() << "Heap allocation detected at: " << CI << ", size: " << allocSize << "\n";
                            }
                            continue;
                        }
                    }
                }

                // 메모리 해제 함수 (free) 호출 감지
                if (CallInst *CI = dyn_cast<CallInst>(&I)) {
                    if (Function *calledFunc = CI->getCalledFunction()) {
                        if (calledFunc->getName() == "free") {
                            Value *freedPtr = CI->getArgOperand(0);
                            removeHeapObject(freedPtr);
                        }
                    }
                }
                

                // load/store 명령어로 힙 오브젝트 접근 감지
                if (LoadInst *LI = dyn_cast<LoadInst>(&I)) {
                    if(checkHeapAccessChanged(LI->getPointerOperand(), &I)){
                        Builder.SetInsertPoint(LI->getNextNode());
                        Builder.CreateCall(configureMPURedzoneForHeapAccess, lastHeapObject);
                    }
                } else if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
                    if(checkHeapAccessChanged(SI->getPointerOperand(), &I)){
                        Builder.SetInsertPoint(SI->getNextNode());
                        Builder.CreateCall(configureMPURedzoneForHeapAccess, lastHeapObject);
                    }
                }
            }
        }

    // 모든 분석 정보를 보존한다고 설정
    return PreservedAnalyses::all();
}


void HeapMPUPass::removeHeapObject(Value *freedPtr) {
    if (heapObjects.count(freedPtr) > 0) {
        heapObjects.erase(freedPtr);
        errs() << "Heap object freed and removed from tracking.\n";
    } else {
        errs() << "Attempt to free untracked memory.\n";
    }
}

bool HeapMPUPass::_isHeapObject( Instruction *I, Value *ptr, Value *heapPtr, uint64_t size) {
    if(!ptr || !heapPtr){
        return false;
    }

    if (auto *gep = dyn_cast<GetElementPtrInst>(ptr)) {
        if (gep->getPointerOperand() == heapPtr) {
            // GEP 기반의 오프셋을 추출하여 범위 확인
            if (auto *constOffset = dyn_cast<ConstantInt>(gep->getOperand(1))) {
                uint64_t offset = constOffset->getZExtValue();
                //errs()<<"offset: "<<offset<<", ptr: "<<ptr<<", heap start: "<<heapPtr<<"\n";
                return offset < size;
            }
        }
    }
    return false;
}


bool HeapMPUPass::checkHeapAccessChanged(Value *currentPtr, Instruction *I) {

    for (auto &[heapPtr, size] : heapObjects) {
            if (_isHeapObject(I, currentPtr, heapPtr, size)) {
                // lastHeapObject가 유효하지 않거나 해제된 경우 초기화
                if (lastHeapObject && heapObjects.find(lastHeapObject) == heapObjects.end()) {
                    lastHeapObject = nullptr;
                }
                
                if (lastHeapObject && lastHeapObject != heapPtr) {
                    errs() << "Accessing a different heap object at: " << *I << "\n";
                    lastHeapObject = heapPtr;
                    return true;
                } else {
                    errs() << "Accessing the same heap object at: " << *I << "\n";
                    return false;
                }
                
            }
        }
    return false;
}


PreservedAnalyses GlobalVariableMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
    
    /////////////////////////////
    // 전역 변수에 REDZONE 삽입 //
    /////////////////////////////

    const int GLOBAL_REDZONE_SIZE = 32;
    Module *M = F.getParent();
    LLVMContext &context = M->getContext();
    Type *int8Ty = Type::getInt8Ty(context);
    ArrayType *redzoneType = ArrayType::get(int8Ty, GLOBAL_REDZONE_SIZE); // 32바이트 레드존 타입
    const DataLayout &dataLayout = F.getParent()->getDataLayout();
    
    // 전역 변수 교체 및 MPU 보호 함수 호출 삽입
    std::vector<GlobalVariable*> globalsToReplace;

    for (GlobalVariable *GV : globalsToReplace) {
        Type *originalType = GV->getValueType();
        uint64_t globalSize = dataLayout.getTypeAllocSize(originalType); // 전역 변수 크기 계산

        if (GV && GV->getName().str().find("_with_redzone") != std::string::npos){
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

    ////////////////////////////
    // 전역 변수 접근 변경 탐지 //
    ////////////////////////////
    for (auto &GV : M->globals()) {
        globalVars.insert(&GV);
        errs() << "Tracking global variable: " << GV.getName() << "\n";
    }
      if (!globalVars.empty()) {
        // set의 마지막 요소 가져오기
        lastGlobalVariable = *std::prev(globalVars.end());
        errs() << "Last element in set: " << lastGlobalVariable <<  "\n";
    }

    Value *globalPointer = nullptr;
    Value *globalBodyPtr = nullptr;
    Value *runtimeBodyPtr =nullptr;
    Value *runtimeBodySize = nullptr;
     for (auto &BB : F) {
            for (auto &I : BB) {
                // load 명령어에서 전역 변수 접근 탐지
                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    if(CheckGlobalVariableAccessChanged(LI->getPointerOperand(), &I)){
                        if (GlobalVariable *GV = istInitializedNonConstantGlobalVariable(LI->getPointerOperand())) {
                            // 전역 변수의 포인터를 가져옴
                            globalPointer = GV;

                            // 크기: 레드존을 포함한 전체 구조체의 크기 계산
                            uint64_t structSize = dataLayout.getTypeAllocSize(GV->getValueType());

                            // 전역 변수 본체의 포인터 추적
                            if (auto *structType = dyn_cast<StructType>(GV->getValueType())) {
                                if (structType->getNumElements() == 3) { // 앞 레드존, 전역 변수, 뒤 레드존
                                    // 구조체의 두 번째 필드를 통해 전역 변수 본체에 접근
                                    globalBodyPtr = ConstantExpr::getInBoundsGetElementPtr(
                                        structType, GV,
                                        ArrayRef<Constant*>{ConstantInt::get(Type::getInt32Ty(GV->getContext()), 0),  // 구조체 기본 주소
                                        ConstantInt::get(Type::getInt32Ty(GV->getContext()), 1)}  // 두 번째 필드
                                    );

                                    // 전역 변수 본체의 크기 계산 (레드존 제외)
                                    uint64_t globalBodySize = dataLayout.getTypeAllocSize(structType->getElementType(1));

                                    // configure_mpu_redzone_for_global 호출 (IRBuilder에서 접근 가능하도록 포인터 준비)
                                    IRBuilder<> builder(LI); // LI 위치에 빌더 설정
                                    runtimeBodyPtr = builder.CreateBitCast(globalBodyPtr, PointerType::get(Type::getInt8Ty(GV->getContext()), 0));
                                    runtimeBodySize = ConstantInt::get(Type::getInt64Ty(GV->getContext()), globalBodySize);
                                    builder.CreateCall(configureMPURedzoneForGlobal, {runtimeBodyPtr, runtimeBodySize});
                                }

                                
                            }

                        }
                    }
                }
                // store 명령어에서 전역 변수 접근 탐지
                else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    if(CheckGlobalVariableAccessChanged(SI->getPointerOperand(), &I)){
                         if (GlobalVariable *GV = istInitializedNonConstantGlobalVariable(SI->getPointerOperand())) {
                            // 전역 변수의 포인터를 가져옴
                            globalPointer = GV;

                            // 크기: 레드존을 포함한 전체 구조체의 크기 계산
                            uint64_t structSize = dataLayout.getTypeAllocSize(GV->getValueType());

                            // 전역 변수 본체의 포인터 추적
                            if (auto *structType = dyn_cast<StructType>(GV->getValueType())) {
                                if (structType->getNumElements() == 3) { // 앞 레드존, 전역 변수, 뒤 레드존
                                    // 구조체의 두 번째 필드를 통해 전역 변수 본체에 접근
                                    globalBodyPtr = ConstantExpr::getInBoundsGetElementPtr(
                                        structType, GV,
                                        ArrayRef<Constant*>{ConstantInt::get(Type::getInt32Ty(GV->getContext()), 0),  // 구조체 기본 주소
                                        ConstantInt::get(Type::getInt32Ty(GV->getContext()), 1)}  // 두 번째 필드
                                    );

                                    // 전역 변수 본체의 크기 계산 (레드존 제외)
                                    uint64_t globalBodySize = dataLayout.getTypeAllocSize(structType->getElementType(1));

                                    // configure_mpu_redzone_for_global 호출 (IRBuilder에서 접근 가능하도록 포인터 준비)
                                    IRBuilder<> builder(LI); // LI 위치에 빌더 설정
                                    Value *runtimeBodyPtr = builder.CreateBitCast(globalBodyPtr, PointerType::get(Type::getInt8Ty(GV->getContext()), 0));
                                    Value *runtimeBodySize = ConstantInt::get(Type::getInt64Ty(GV->getContext()), globalBodySize);
                                    builder.CreateCall(configureMPURedzoneForGlobal, {runtimeBodyPtr, runtimeBodySize});
                                }

                                
                            }

                        }
                    }
                }
            }
        }
    return PreservedAnalyses::all();
}

  
bool GlobalVariableMPUPass::isGlobalVariable(Value *ptr){
    // 전역 변수라면 GlobalVariable로 캐스팅 가능
    if (auto *GV = dyn_cast<GlobalVariable>(ptr)) {
        return globalVars.count(GV) > 0;
    }
    return false;
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

bool GlobalVariableMPUPass::CheckGlobalVariableAccessChanged(Value *currentPtr, Instruction *I){
    if (isGlobalVariable(currentPtr)) {
        GlobalVariable* currentGlobalVariable = dyn_cast<GlobalVariable>(currentPtr);
        // lastGlobalVariable 유효하지 않거나 해제된 경우 초기화
        if (lastGlobalVariable && globalVars.find(lastGlobalVariable) == globalVars.end()) {
            lastGlobalVariable = nullptr;
        }
        errs()<<"last GV: "<<lastGlobalVariable<<", cur GV: "<<currentGlobalVariable<<"\n";
        if (lastGlobalVariable && lastGlobalVariable != currentGlobalVariable) {
            errs() << "Accessing a different global variable at: " << *I << "\n";
            lastGlobalVariable = currentGlobalVariable;
            return true;
        } else {
            errs() << "Accessing the same global object at: " << *I << "\n";
            return false;
        }
        
    }
    
    return false;
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