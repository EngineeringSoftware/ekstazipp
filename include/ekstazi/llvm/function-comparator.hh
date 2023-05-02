
#pragma once

#include "llvm/Transforms/Utils/FunctionComparator.h"

namespace ekstazi
{

class FunctionComparator : private llvm::FunctionComparator
{
public:
    class HashAccumulator
    {
    public:
        // Initialize to random constant, so the state isn't zero.
        HashAccumulator();
        
        void add(uint64_t v);

        void add(std::string const & v);

        // Hash LLVM constant values
        void add(llvm::Constant const * const_val);

        template <typename T>
        void add(T & v);
        
        // No finishing is required, because the entire hash value is used.
        uint64_t get_hash();

    protected:
        uint64_t hash;
    };
    static FunctionComparator::FunctionHash functionHash(llvm::Function &F);
};

}
