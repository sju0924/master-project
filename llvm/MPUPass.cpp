#include "MPUPass.h"

PreservedAnalyses StackMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  errs() << "Analyzing function: " << F.getName() << "\n";

    IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

    // RSP 조정을 위한 어셈블리 코드 삽입
    // Inline assembly to adjust rsp by 64 bytes (redzone)
    InlineAsm *SubRSP = InlineAsm::get(
        FunctionType::get(Type::getVoidTy(Ctx), false),
        "sub rsp, 64", "", true);

    InlineAsm *AddRSP = InlineAsm::get(
        FunctionType::get(Type::getVoidTy(Ctx), false),
        "add rsp, 64", "", true);


    for (auto &BB : F) {
        for (auto I = BB.begin(); I != BB.end(); ++I) {
            // CallInst를 통해 함수 호출 감지
            if (CallInst *CI = dyn_cast<CallInst>(&*I)) {
                // 함수 호출 전: "sub rsp, 64" 삽입
                IRBuilder<> BuilderBefore(CI);
                BuilderBefore.CreateCall(SubRSP);

                // 함수 호출 후: "add rsp, 64" 삽입
                ++I; // Move iterator to the next instruction
                if (I != BB.end()) { // Check if iterator is still valid
                    IRBuilder<> BuilderAfter(&*I);
                    BuilderAfter.CreateCall(AddRSP);
                }
                --I; // Restore iterator to point at the original call

                if (Function *calledFunc = CI->getCalledFunction()) {
                    errs() << "  Function call detected: " 
                           << F.getName() << " calls " 
                           << calledFunc->getName() << "\n";
                } else {
                    errs() << "  Indirect function call detected in function: " 
                           << F.getName() << "\n";
                }
            }

            // Return analysis: ReturnInst를 통해 함수 리턴 감지
            if (ReturnInst *Ret = dyn_cast<ReturnInst>(BB.getTerminator())) {
                IRBuilder<> RetBuilder(Ret);
                RetBuilder.CreateLoad(RedZoneAlloc);  // 복구 로직 추가
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
    LLVMContext &context = F.getContext();
    const DataLayout &dataLayout = F.getParent()->getDataLayout();


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
                    checkHeapAccessChanged(LI->getPointerOperand(), &I);
                } else if (StoreInst *SI = dyn_cast<StoreInst>(&I)) {
                    checkHeapAccessChanged(SI->getPointerOperand(), &I);
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
    
     Module *M = F.getParent();

    for (auto &GV : M->globals()) {
        globalVars.insert(&GV);
        errs() << "Tracking global variable: " << GV.getName() << "\n";
    }
      if (!globalVars.empty()) {
        // set의 마지막 요소 가져오기
        lastGlobalVariable = *std::prev(globalVars.end());
        errs() << "Last element in set: " << lastGlobalVariable <<  "\n";
    }


     for (auto &BB : F) {
            for (auto &I : BB) {
                // load 명령어에서 전역 변수 접근 탐지
                if (auto *LI = dyn_cast<LoadInst>(&I)) {
                    CheckGlobalVariableAccessChanged(LI->getPointerOperand(), &I);
                }
                // store 명령어에서 전역 변수 접근 탐지
                else if (auto *SI = dyn_cast<StoreInst>(&I)) {
                    CheckGlobalVariableAccessChanged(SI->getPointerOperand(), &I);
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