
#include "ekstazi/llvm/function-comparator.hh"
#include "ekstazi/test-frameworks/gtest/gtest-adapter.hh"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <string>
#include <iterator>

using namespace llvm;

namespace ekstazi
{

FunctionComparator::HashAccumulator::HashAccumulator()
{
    hash = 0x6acaa36bef8325c5ULL;
}

void FunctionComparator::HashAccumulator::add(uint64_t v)
{
    hash = hashing::detail::hash_16_bytes(hash, v);
}

void FunctionComparator::HashAccumulator::add(std::string const & v)
{
    hash += hash_value(v);
}

// Hash LLVM constant values
void FunctionComparator::HashAccumulator::add(llvm::Constant const * const_val)
{
    const_val = const_val->stripPointerCasts();
    // Constant* const_val = dyn_cast<Constant>(operand_val);
    /**
     * ConstantData represents const data values. Our goal is to retrieve the actual value of
     * the const value if possible and then hash it.
     */
    if (isa<ConstantData>(const_val))
    {
        // For constant ints, we simply hash the value of the integer.
        if (isa<ConstantInt>(const_val))
        {
            ConstantInt const * const_int = dyn_cast<ConstantInt>(const_val);
            add(*(const_int->getValue().getRawData()));
            // errs() << *(const_int->getValue().getRawData()) << '\n';
        }

        // For fp, we try to get the value as a double and then hash
        else if (isa<ConstantFP>(const_val))
        {
            ConstantFP const * const_fp = dyn_cast<ConstantFP>(const_val);
            add(const_fp->getValueAPF().convertToDouble());
        }

        // For strings, we need to get the bytes of the array
        else if (isa<ConstantDataSequential>(const_val))
        {
            ConstantDataSequential const * const_arr = dyn_cast<ConstantDataSequential>(const_val);
            add(const_arr->getRawDataValues().str());
        }
    }

    /**
     * ConstantExpr represents constexpr's, which are initialized using other constant values.
     */
    else if (isa<ConstantExpr>(const_val))
    {

    }

    /**
     * ConstantAggregate represents aggregate data types, which include structs and arrays.
     */
    else if (isa<ConstantAggregate>(const_val))
    {

    }

    /**
     * GlobalValue represents global objects (functions and variables) and symbols (aliases).
     */
    else if (isa<GlobalValue>(const_val))
    {
        GlobalValue const * gv = dyn_cast<GlobalValue>(const_val);
        // add(gv->getGUID());

        if (isa<GlobalVariable>(const_val))
        {
            GlobalVariable const * global_var = dyn_cast<GlobalVariable>(const_val);
            if (gtest::GtestAdapter::is_internal_function(global_var->getName()))
            {
                return;
            }
            if (global_var->hasInitializer())
            {
                // errs() << global_var->getName() << '\n';
                Constant const * init_val = global_var->getInitializer();
                add(init_val);
            }
            
        }
    }
}

template <typename T>
void FunctionComparator::HashAccumulator::add(T & v)
{
    hash += hash_value(v);
}

// No finishing is required, because the entire hash value is used.
uint64_t FunctionComparator::HashAccumulator::get_hash()
{
    return hash;
}

FunctionComparator::FunctionHash FunctionComparator::functionHash(llvm::Function &F)
{
    HashAccumulator H;
    H.add(F.isVarArg());
    H.add(F.arg_size());
    
    SmallVector<const BasicBlock *, 8> BBs;
    SmallSet<const BasicBlock *, 16> VisitedBBs;

    // Walk the blocks in the same order as
    // FunctionComparator::cmpBasicBlocks(), accumulating the hash of
    // the function "structure." (BB and opcode sequence).
    BBs.push_back(&F.getEntryBlock());
    VisitedBBs.insert(BBs[0]);
    while (!BBs.empty()) {
        const BasicBlock *BB = BBs.pop_back_val();
        
        // This random value acts as a block header, as otherwise the
        // partition of opcodes into BBs wouldn't affect the hash,
        // only the order of the opcodes.
        H.add(45798);
        for (auto &Inst : *BB) {
            H.add(Inst.getOpcode());

            // errs() << F.getName() << ';' << Inst.getOpcodeName() << '\n';

            // Ignore call target offsetse
            if (isa<CallInst>(Inst) || isa<InvokeInst>(Inst))
            {
                if (isa<CallInst>(Inst))
                {
                    CallInst const * call_inst = dyn_cast<CallInst>(&Inst);
                    llvm::Function* called_fun = call_inst->getCalledFunction();
                    if (called_fun)
                    {
                        // Skip the internal gtest functions that depend on code location of test
                        if (ekstazi::gtest::GtestAdapter::is_internal_function(called_fun->getName()))
                        {
                            continue;
                        }
                    }
                    for (Use const & arg : call_inst->arg_operands())
                    {
                        Value* val = arg.get();
                        if (dyn_cast<llvm::Function>(val))
                        {
                            continue;
                        }
                        if (isa<Constant>(val))
                        {
                            H.add(dyn_cast<Constant>(val));
                        }
                    }
                }

                else
                {
                    InvokeInst const * invoke_inst = dyn_cast<InvokeInst>(&Inst);
                    llvm::Function* called_fun = invoke_inst->getCalledFunction();
                    if (called_fun)
                    {
                        // Skip the internal gtest functions that depend on code location of test
                        if (ekstazi::gtest::GtestAdapter::is_internal_function(called_fun->getName()))
                        {
                            continue;
                        }
                    }
                    for (Use const & arg : invoke_inst->arg_operands())
                    {
                        Value* val = arg.get();
                        if (dyn_cast<llvm::Function>(val))
                        {
                            continue;
                        }
                        if (isa<Constant>(val))
                        {
                            H.add(dyn_cast<Constant>(val));
                        }
                    }
                }
                continue;
            }

            // I tried here to get values for constants, but how do
            // they store constants that are numbers (as constants can
            // be many other things)?
            // for (uint32_t i = 0; i < Inst.getNumOperands(); i++) {
            //     Value *Val = Inst.getOperand(i);
            //     if (isa<Constant>(*Val)) {
            //         Constant *Const = (Constant*) Val;
            //         if (Const->isOneValue()) {
            //             std::cout << " " << Const->getUniqueInteger().toString(10, false) << std::endl;
            //         }
            //     }
            // }
            for (Value const * operand_val : Inst.operand_values())
            {
                // Value* operand_val = operand.get();

                // H.add(operand_val->getValueID());
                // std::string val_str;
                // raw_string_ostream rso{ val_str };
                // operand_val->print(rso);
                // errs() << rso.str() << '\n';
                // H.add(hashing::detail::hash_short(rso.str().c_str(), rso.str().length(), 0));
                // std::hash<std::string>(operand);
                if (isa<Constant>(operand_val))
                {
                    H.add(dyn_cast<Constant>(operand_val));
                }
            }
        }

        const TerminatorInst *Term = BB->getTerminator();
        for (unsigned i = 0, e = Term->getNumSuccessors(); i != e; ++i) {
            if (!VisitedBBs.insert(Term->getSuccessor(i)).second) {
                continue;
            }
            BBs.push_back(Term->getSuccessor(i));
        }
    }

    return H.get_hash();
    // FunctionComparator::FunctionHash h = llvm::FunctionComparator::functionHash(F);
    // return h;
}

}
