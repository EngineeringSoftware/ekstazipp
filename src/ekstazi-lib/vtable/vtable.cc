#include "ekstazi/vtable/vtable.hh"

#include "ekstazi/utils/mangle.hh"

#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace ekstazi
{

std::string const VTable::CXX_ABI_VTABLE_PREFIX{ "vtable for " };

bool VTable::is_vtable_def(GlobalVariable & gv)
{
    // First, vtable definitions contain the mangled name 'TV'
    std::string const & name = gv.getName().str();
    // errs() << name << '\n';

    if (name.find("TV") == std::string::npos)
    {
        return false;
    }

    if (!gv.hasInitializer())
    {
        return false;
    }

    Constant* initialized_val = gv.getInitializer();
    // errs() << *initialized_val << '\n';
    // errs() << (initialized_val->getType()->getStructName()) << '\n';

    Constant* vtable_arr = initialized_val->getAggregateElement((unsigned)0);

    // errs() << *vtable_arr << '\n';


    // ConstantDataArray* arr = dyn_cast<ConstantDataArray>(initialized_val);
    // if (!arr)
    // {
    //     return false;
    // }
    if (vtable_arr == nullptr)
    {
        return false;
    }

    ConstantArray* vtable_const_arr = dyn_cast<ConstantArray>(vtable_arr);
    if (vtable_const_arr == nullptr)
    {
        return false;
    }

    return true;

}

bool VTable::is_vtable_def(GlobalVariable* gv)
{
    return VTable::is_vtable_def(*gv);
}

/**
 * Strip the 'vtable for ' string from the demangled vtable name.
 */
std::string & VTable::strip_prefix_from_vtable_name(std::string & name)
{
    size_t pos = name.find(CXX_ABI_VTABLE_PREFIX);
    if (pos != std::string::npos)
    {
        return name.erase(pos, CXX_ABI_VTABLE_PREFIX.size());
    }
    return name;
    
}

VTable::VTable()
{

}


void VTable::add_entries(GlobalVariable* vt)
{
    if (!VTable::is_vtable_def(vt))
    {
        return;
    }
    // Get name of virtual table type. We need to strip the prefix after demangling.
    m_name = demangle(vt->getName());
    m_name = strip_prefix_from_vtable_name(m_name);
    // errs() << "Name of vtable type: " << m_name << '\n';

    ConstantArray* vtable_arr = dyn_cast<ConstantArray>(vt->getInitializer()->getAggregateElement((unsigned)0));

    // First parse the offset
    Constant* offset = vtable_arr->getAggregateElement((unsigned)0);
    // errs() << "Offset: " << *(offset->stripPointerCasts()) << '\n';

    // vtable[1] is always the RTTI, if available
    Constant* rtti = vtable_arr->getAggregateElement((unsigned)1);
    // errs() << "RTTI: " << *(rtti->stripPointerCasts()) << '\n';

    // Now parse all of the virtual function pointers
    uint32_t i = 2;
    Constant* vtable_entry = vtable_arr->getAggregateElement(i);

    // The first two entries of the vtable are the offset and the RTTI.
    while (vtable_entry)
    {
        
        // errs() << *vtable_entry << '\n';
        // errs() << *(vtable_entry->stripPointerCasts()->getType()) << '\n';
        Function* virt_fun = dyn_cast<Function>(vtable_entry->stripPointerCasts());
        if (virt_fun)
        {
            // errs() << virt_fun->getName() << '\n';
            m_vfuns.push_back(virt_fun);
        }

        ++i;
        vtable_entry = vtable_arr->getAggregateElement(i);
    }
    
    // errs() << "Num vtable entries: " << i << '\n';

    // for (std::string & fun : m_vfuns)
    // {
    //     errs() << fun << '\n';
    // }
}

std::string VTable::get_name() const
{
    return m_name;
}

std::vector<Function*> const & VTable::get_vfuns() const
{
    return m_vfuns;
}


}
