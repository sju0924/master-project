#include "llvm/Passes/PassPlugin.h"  // 새로운 패스 관리 시스템 헤더
#include "llvm/Passes/PassBuilder.h" // PassBuilder 정의
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/PassManager.h" 
#include "llvm/IR/Module.h"  
#include "llvm/IR/IRBuilder.h" 
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DataLayout.h"
#include <set>


using namespace llvm;

namespace {

struct StackMPUPass : public PassInfoMixin<StackMPUPass> {
    // call analysis (push stack frame)
    // return analysis (pop stack frame)
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "StackMPUPass"; }
};

struct HeapMPUPass : public PassInfoMixin<HeapMPUPass> {
    // Find where heap object in-use has been changed
    // locate the redzone of the heap object
    // set mpu config of over/under redzone of the heap object
private:
    // 힙 오브젝트 포인터를 저장할 집합
    std::map<Value*, uint64_t> heapObjects;
    Value *lastHeapObject = nullptr;

/* 
    @description: 해제된 힙 오브젝트를 heapObjects에서 제거
    @input 
        - Value *freedPtr: 해제할 힙 오브젝트
*/
    void removeHeapObject(Value *freedPtr);

/* 
    @description: 어떠한 주소가 특정 힙 오브젝트인지 확인하는 함수
    @input 
        - Instruction *I: 검사 대상이 되는 명령어
        - Value *ptr: 검사 대상이 되는 주소
        - Value *heapPtr: 힙 오브젝트의 시작 주소
        - uint64_t size: 힙 오브젝트의 크기
    @output 
        - 1: ptr이 해당 힙 오브젝트임 
        - 0: ptr이 해당 힙 오브젝트가 아님 
*/
    bool _isHeapObject(Instruction *I, Value *ptr, Value *heapPtr, uint64_t size);

/* 
    @description: 이전과 다른 힙 오브젝트에 접근하는지 확인하는 함수
    @input 
        - Value *currentPtr: 검사 대상이 되는 힙 메모리 주소
        - Instruction *I: 힙 오브젝트에 접근하는 명령어
    @output 
        - 1: 이전과 다른 힙 오브젝트에 접근함
        - 0: 이전과 같은 힙 오브젝트에 접근함
*/
    bool checkHeapAccessChanged(Value *currentPtr, Instruction *I);

public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "HeapMPUPass"; }
};

struct GlobalVariableMPUPass : public PassInfoMixin<GlobalVariableMPUPass> {
    // Find where global object in-use has been changed
    // locate the redzone of the global object
    // set mpu config of over/under redzone of the global object
public: 

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static StringRef name() { return "GlobalVariableMPUPass"; }

private:
    std::set<GlobalVariable*> globalVars;  // 추적할 전역 변수들 저장
    GlobalVariable *lastGlobalVariable = nullptr;

     /*
        @description: 이전과 다른 전역 변수에 접근하는지 확인하는 함수
        @input
            - Value *ptr: 검사 대상 포인터
        @output
            - 1: 이전과 다른 전역 변수에 접근함
            - 0: 이전과 같은 전역 변수에 접근함
    */ 
    bool CheckGlobalVariableAccessChanged(Value *currentPtr, Instruction *I);

    /*
        @description: 포인터가 전역 변수인지 확인
        @input
            - Value *ptr: 검사 대상 포인터
        @output
            - 1: 포인터가 전역 변수임
            - 0: 포인터가 전역 변수가 아님
    */ 
    bool isGlobalVariable(Value *ptr);
 
}; 

} // namespace 