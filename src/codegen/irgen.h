// Copyright (c) 2014 Dropbox, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PYSTON_CODEGEN_IRGEN_H
#define PYSTON_CODEGEN_IRGEN_H

#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IRBuilder.h"

#include "core/types.h"

namespace pyston {

class AST_expr;
class AST_stmt;
class GCBuilder;
class IREmitter;

struct UnwindInfo {
public:
    AST_stmt* current_stmt;
    llvm::BasicBlock* exc_dest;

    bool needsInvoke() { return exc_dest != NULL; }

    UnwindInfo(AST_stmt* current_stmt, llvm::BasicBlock* exc_dest) : current_stmt(current_stmt), exc_dest(exc_dest) {}

    // Risky!  This means that we can't unwind from this location, and should be used in the
    // rare case that there are language-specific reasons that the statement should not unwind
    // (ex: loading function arguments into the appropriate scopes).
    static UnwindInfo cantUnwind() { return UnwindInfo(NULL, NULL); }
};

// TODO get rid of this
class MyInserter : public llvm::IRBuilderDefaultInserter<true> {
private:
    IREmitter* emitter;

protected:
    void InsertHelper(llvm::Instruction* I, const llvm::Twine& Name, llvm::BasicBlock* BB,
                      llvm::BasicBlock::iterator InsertPt) const;

public:
    void setEmitter(IREmitter* emitter) { this->emitter = emitter; }
};

class ICSetupInfo;

class IREmitter {
public:
    typedef llvm::IRBuilder<true, llvm::ConstantFolder, MyInserter> IRBuilder;

    virtual ~IREmitter() {}

    virtual IRBuilder* getBuilder() = 0;
    virtual GCBuilder* getGC() = 0;
    virtual CompiledFunction* currentFunction() = 0;
    virtual llvm::BasicBlock* createBasicBlock(const char* name = "") = 0;

    virtual llvm::Value* getScratch(int num_bytes) = 0;
    virtual void releaseScratch(llvm::Value*) = 0;

    virtual llvm::Function* getIntrinsic(llvm::Intrinsic::ID) = 0;

    virtual llvm::Value* createCall(UnwindInfo unw_info, llvm::Value* callee, const std::vector<llvm::Value*>& args)
        = 0;
    virtual llvm::Value* createCall(UnwindInfo unw_info, llvm::Value* callee) = 0;
    virtual llvm::Value* createCall(UnwindInfo unw_info, llvm::Value* callee, llvm::Value* arg1) = 0;
    virtual llvm::Value* createCall2(UnwindInfo unw_info, llvm::Value* callee, llvm::Value* arg1, llvm::Value* arg2)
        = 0;
    virtual llvm::Value* createCall3(UnwindInfo unw_info, llvm::Value* callee, llvm::Value* arg1, llvm::Value* arg2,
                                     llvm::Value* arg3) = 0;
    virtual llvm::Value* createIC(const ICSetupInfo* pp, void* func_addr, const std::vector<llvm::Value*>& args,
                                  UnwindInfo unw_info) = 0;
};

extern const std::string CREATED_CLOSURE_NAME;
extern const std::string PASSED_CLOSURE_NAME;
extern const std::string PASSED_GENERATOR_NAME;

std::string getIsDefinedName(const std::string& name);
bool isIsDefinedName(const std::string& name);

CompiledFunction* doCompile(SourceInfo* source, const OSREntryDescriptor* entry_descriptor,
                            EffortLevel::EffortLevel effort, FunctionSpecialization* spec, std::string nameprefix);

// A common pattern is to branch based off whether a variable is defined but only if it is
// potentially-undefined.  If it is potentially-undefined, we have to generate control-flow
// that branches on the is_defined variable and then generate different code on those two paths;
// if the variable is guaranteed to be defined, we just want to emit the when_defined version.
//
// I suppose we could always emit both and let the LLVM optimizer fix it up for us, but for now
// do it the hard (and hopefully faster) way.
//
// - is_defined_var is allowed to be NULL, signifying that the variable is always defined.
//   Otherwise it should be a BOOL variable that signifies if the variable is defined or not.
// - speculate_undefined means whether or not we should execute the when_undefined code generator
//   in the current block (the one that we're in when calling this function); if set to true we will
//   avoid generating a BB for the undefined case, which is useful if the "codegen" just returns
//   an existing value or a constant.
llvm::Value* handlePotentiallyUndefined(ConcreteCompilerVariable* is_defined_var, llvm::Type* rtn_type,
                                        llvm::BasicBlock*& cur_block, IREmitter& emitter, bool speculate_undefined,
                                        std::function<llvm::Value*(IREmitter&)> when_defined,
                                        std::function<llvm::Value*(IREmitter&)> when_undefined);

class TypeRecorder;
class OpInfo {
private:
    const EffortLevel::EffortLevel effort;
    TypeRecorder* const type_recorder;

public:
    const UnwindInfo unw_info;

    OpInfo(EffortLevel::EffortLevel effort, TypeRecorder* type_recorder, UnwindInfo unw_info)
        : effort(effort), type_recorder(type_recorder), unw_info(unw_info) {}

    bool isInterpreted() const { return effort == EffortLevel::INTERPRETED; }
    TypeRecorder* getTypeRecorder() const { return type_recorder; }
};
}

#endif
