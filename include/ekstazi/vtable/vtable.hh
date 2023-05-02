
#pragma once

#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <memory>

namespace ekstazi
{

/**
 * Ekstazi Virtual Table representation.
 * 
 * VTables generated in LLVM follow the specification defined in the Itanium CXX ABI:
 * https://itanium-cxx-abi.github.io/cxx-abi/abi.html#vtable
 * 
 * Additionally, the LLVM reference for type metadata:
 * https://llvm.org/docs/TypeMetadata.html
 * 
 * VTables have the following layout:
 * 
 * index    |   value
 * ------------------------------
 * 0        |   offset
 * ------------------------------
 * 1        |   rtti
 * ------------------------------
 * 2        |   address of virtual function 1
 * ------------------------------
 * 3        |   address of virtual function 2
 * ------------------------------
 * 4        |   ...
 * ------------------------------
 * 
 * Thus, vtable[0] will always contain the offset for the type, and
 * vtable[1] will always contain the rtti, if available.
 * 
 * For ekstazi, we simplify the VTable implementation by storing:
 *  1. The name of the type that the VTable is for (string)
 *  2. The offset (uint32_t)
 *  3. RTTI (string)
 *  4. Vector of virtual functions (string[])
 * 
 * Since virtual calls in LLVM IR will use an offset based on the
 * virtual functions index rather than the actual vtable index,
 * this makes retrieving the correct virtual function pointer easy,
 * as we do not have to modify the offset at all.
 * 
 * 
 */
class VTable
{
public:
    static std::string const CXX_ABI_VTABLE_PREFIX;

    static bool is_vtable_def(llvm::GlobalVariable & gv);
    static bool is_vtable_def(llvm::GlobalVariable* gv);

    /**
     * Strip the 'vtable for ' string from the demangled vtable name.
     */
    static std::string & strip_prefix_from_vtable_name(std::string & name);

    VTable();

    /**
     * Add all VT entries from a Global Variable value.
     */
    void add_entries(llvm::GlobalVariable* vt);

    std::string get_name() const;

    uint32_t get_offset() const;
    std::string const & get_rtti() const;

    std::vector<llvm::Function*> const & get_vfuns() const;

protected:
    // Name of the type that this vtable is for
    std::string m_name;

    uint32_t m_offset;
    std::string m_rtti;

    std::vector<llvm::Function*> m_vfuns;
};


}
