#include "MPUPass.h"

PreservedAnalyses StackMPUPass::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  errs() << "Analyzing function: " << F.getName() << "\n";

    for (auto &BB : F) {
        for (auto &I : BB) {
            // Call analysis: CallInst를 통해 함수 호출 감지
            if (CallInst *CI = dyn_cast<CallInst>(&I)) {
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
            if (isa<ReturnInst>(&I)) {
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
    // To be implemented
    return PreservedAnalyses::all();
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