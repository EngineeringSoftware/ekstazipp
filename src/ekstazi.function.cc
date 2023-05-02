#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ValueSymbolTable.h"

#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Transforms/Utils/FunctionComparator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Casting.h"

#include "llvm/Demangle/Demangle.h"

#include "llvm/Support/CommandLine.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <cxxabi.h>
#include <cstdio>
#include <unordered_set>
#include <map>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <memory>

// Hash checksum implementation
#include "sha512.h"

#include "ekstazi/constants.hh"

#include "ekstazi/depgraph/depgraph.hh"
#include "ekstazi/depgraph/function.hh"

#include "ekstazi/type-hierarchy/type-hierarchy.hh"

#include "ekstazi/test-frameworks/gtest/gtest-adapter.hh"

#include "ekstazi/llvm/function-comparator.hh"

#include "ekstazi/vtable/vtable.hh"
#include "ekstazi/utils/mangle.hh"

using namespace llvm;

namespace
{

// Test executable name
static cl::opt<std::string> test_exec_fname{ "test-executable", cl::desc("Specify test executable"), cl::value_desc("test filename") };

class Ekstazi : public FunctionPass
{
protected:
    // Ekstazi Gtest Adapter
    ekstazi::gtest::GtestAdapter gtest_adapter;

    // Path to the bitcode
    std::string bc_fname;

    // Module name
    std::string module_name;

    std::string count_fname;

    // Type hierarchy
    std::string old_type_hierarchy_fname;
    ekstazi::TypeHierarchy old_type_hierarchy;

    std::string new_type_hierarchy_fname;
    ekstazi::TypeHierarchy new_type_hierarchy;

    // Dependency graphs
    std::string old_depgraph_fname;
    ekstazi::DependencyGraph old_depgraph;

    std::string new_depgraph_fname;
    ekstazi::DependencyGraph new_depgraph;

    // Set of Functions
    std::string old_functions_fname;
    std::map<std::string, ekstazi::Function> old_functions;

    std::string new_functions_fname;
    std::map<std::string, ekstazi::Function> new_functions;

    // Set of Constructors
    std::string old_constructors_fname;
    std::unordered_set<std::string> old_constructors;

    std::string new_constructors_fname;
    std::unordered_set<std::string> new_constructors;

    // Set of Modified Functions
    std::string modified_functions_fname;
    std::unordered_set<std::string> modified_functions;

    // Tests should be the leaf nodes of our graph
    std::string modified_tests_fname;
    std::unordered_set<std::string> modified_tests;

    // Virtual Tables for all classes
    // {key, val} = {Class Name, VTable for Class}
    std::unordered_map<std::string, std::shared_ptr<ekstazi::VTable>> vtables;

    // Virtual Function calls
    // {caller, callee}
    std::vector<std::pair<Function*, Function*>> virtual_calls;

    // Profiling tools
    class Timer
    {
    public:
        auto start()
        {
            timer_start = std::chrono::high_resolution_clock::now();
            return timer_start;
        }

        auto end()
        {
            timer_end = std::chrono::high_resolution_clock::now();
            return timer_end;
        }

        auto get_duration()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(timer_end - timer_start).count();
        }

    protected:
        std::chrono::time_point<std::chrono::high_resolution_clock> timer_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> timer_end;
    };

    Timer timer;

public:
    static char ID;

    Ekstazi() : FunctionPass(ID) {}

    bool doInitialization(Module &M) override
    {
        timer.start();
        // Module name
        module_name = M.getName();
        bc_fname = module_name;
        // Strip any directory paths so we get the plain module name
        size_t last_dir_index = module_name.find_last_of('/');
        if (last_dir_index != std::string::npos)
        {
            module_name = module_name.substr(last_dir_index + 1, module_name.length());
        }

        // Initial indicator file filename
        count_fname = ekstazi::EKSTAZI_DIRNAME + '/' + ekstazi::COUNT_FNAME;

        new_type_hierarchy_fname = ekstazi::EKSTAZI_DIRNAME + '/' + module_name + '.' + ekstazi::TYPE_HIERARCHY_FNAME;
        old_type_hierarchy_fname = new_type_hierarchy_fname + '.' + ekstazi::OLD_SUFFIX;

        // Check for existing class hierarchy
        std::ifstream ifs{ new_type_hierarchy_fname };
        if (ifs)
        {
            errs() << "Renaming type hierarchy file to: " << old_type_hierarchy_fname << '\n';
            std::rename(new_type_hierarchy_fname.c_str(), old_type_hierarchy_fname.c_str());

            // Load the old class hierarchy
            old_type_hierarchy.load_file(old_type_hierarchy_fname);
        }
        ifs.close();

        new_depgraph_fname = ekstazi::EKSTAZI_DIRNAME + '/' + module_name + '.' + ekstazi::DEPGRAPH_FNAME;
        new_depgraph = ekstazi::DependencyGraph{};

        old_depgraph_fname = new_depgraph_fname + '.' + ekstazi::OLD_SUFFIX;
        old_depgraph = ekstazi::DependencyGraph{};

        // Check for existing dependency graph
        ifs = std::ifstream{ new_depgraph_fname };
        if (ifs)
        {
            errs() << "Renaming dependency file to: " << old_depgraph_fname << '\n';
            std::rename(new_depgraph_fname.c_str(), old_depgraph_fname.c_str());

            // Load the old dependency graph
            old_depgraph.load_file(old_depgraph_fname);
        }
        ifs.close();

        new_functions_fname = ekstazi::EKSTAZI_DIRNAME + '/' + module_name + '.' + ekstazi::FUNCTIONS_FNAME;
        new_functions = std::map<std::string, ekstazi::Function>{};

        old_functions_fname = new_functions_fname + '.' + ekstazi::OLD_SUFFIX;
        old_functions = std::map<std::string, ekstazi::Function>{};

        // Check for existing function checksums file
        ifs = std::ifstream{ new_functions_fname };
        if (ifs)
        {
            errs() << "Renaming function checksums file to: " << old_functions_fname << '\n';
            std::rename(new_functions_fname.c_str(), old_functions_fname.c_str());

            // Load the old function checksums
            old_functions = ekstazi::Function::load_file(old_functions_fname);
        }
        ifs.close();

        modified_functions_fname = ekstazi::EKSTAZI_DIRNAME + "/" + module_name + "." + ekstazi::MODIFIED_FUNS_FNAME;
        modified_functions = std::unordered_set<std::string>{};

        modified_tests_fname = ekstazi::EKSTAZI_DIRNAME + "/" + module_name + "." + ekstazi::TESTS_FNAME;
        modified_tests = std::unordered_set<std::string>{};

        build_class_hierarchy(M);

        build_vtables(M);

        timer.end();
        errs() << "Time for initialization: " << timer.get_duration() << " ms\n";

        timer.start();

        return false;
    }


    bool runOnFunction(Function &F) override
    {
        for (BasicBlock & BB : F)
        {
            for (Instruction &I : BB)
            {
                if (!isa<CallInst>(I) && !isa<InvokeInst>(I))
                {
                    continue;
                }

                Function* caller = &F;

                // Make sure caller is not null and is in this module
                if (caller == nullptr || caller->isDeclaration())
                {
                    continue;
                }

                // Append to the set of functions
                add_to_function_set(caller);

                CallInst* ci = dyn_cast<CallInst>(&I);
                InvokeInst* ii = dyn_cast<InvokeInst>(&I);

                Function* callee;
                if (ci)
                {
                    callee = ci->getCalledFunction();
                }
                else
                {
                    callee = ii->getCalledFunction();
                }

                // If callee is null, then is an indirect call
                if (callee == nullptr)
                {
                    Value* call_value;
                    if (ci)
                    {
                        call_value = ci->getCalledValue();
                    }
                    else
                    {
                        call_value = ii->getCalledValue();
                    }
                    // If this is a virtual call, the preceding instruction has to be a load
                    if (LoadInst* li = dyn_cast<LoadInst>(call_value))
                    {
                        Value* load_val = li->getPointerOperand();
                        if (GetElementPtrInst* gepi = dyn_cast<GetElementPtrInst>(load_val))
                        {
                            // A VTable access will always be 1 index
                            // errs() << "Virtual call with offset " << *gepi << '\n';
                            if (gepi->getNumIndices() != 1)
                            {
                                continue;
                            }

                            // errs() << *(gepi->getOperand(0)) << '\n';
                            Type* type = gepi->getSourceElementType();
                            // errs() << type->isPointerTy() << '\n';
                            // errs() << *(dyn_cast<PointerType>(type)->getElementType()) << '\n';

                            FunctionType* call_type = dyn_cast<FunctionType>(dyn_cast<PointerType>(type)->getElementType());
                            PointerType* vtable_type = dyn_cast<PointerType>(call_type->getParamType(0));
                            // errs() << *(vtable_type->getPointerElementType()) << '\n';
                            StructType* class_type = dyn_cast<StructType>(vtable_type->getPointerElementType());

                            // strip 'class.' from name
                            std::string class_name = class_type->getStructName();
                            // errs() << "VTable class: " << class_name << '\n';
                            class_name.erase(0, std::string{"class."}.size());
                            std::unordered_set<std::string> related_classes = new_type_hierarchy.get_derived_types(class_name);
                            // errs() << "Related classes: " << related_classes.size() << '\n';
                            // for (auto & c : related_classes)
                            // {
                            //     errs() << c << '\n';
                            // }
                            
                            // errs() << class_name << '\n';

                            // Get the index of the vtable offset
                            for (Use & idx : gepi->indices())
                            {
                                if (ConstantInt* const_idx = dyn_cast<ConstantInt>(idx))
                                {
                                    // errs() << "VTable offset: " << const_idx->getZExtValue() << ',';
                                    uint64_t index = const_idx->getZExtValue();
                                    auto it = vtables.find(class_name);
                                    if (it == vtables.end())
                                    {
                                        // errs() << "Not found: " << class_name << '\n';
                                        continue;
                                    }
                                    std::shared_ptr<ekstazi::VTable> vtable = it->second;
                                    // errs() << "Called class: " << vtable->get_name() << '\n';
                                    std::vector<Function*> const & vfuns = vtable->get_vfuns();
                                    if (index > vfuns.size() - 1)
                                    {
                                        // errs() << "VTable offset " << index << " is larger than vtable size: " << vfuns.size() << '\n';
                                        continue;
                                    }
                                    // errs() << "Called vfunc: " << vfuns[index] << '\n';
                                    Function* callee = vfuns[index];
                                    // errs() << "Called vfunc: " << callee->getName() << '\n';
                                    std::string callee_name = callee->getName();
                                    // Ignore pure virtual functions
                                    if (callee_name.find("__cxa_pure_virtual") == std::string::npos)
                                    {
                                        add_to_function_set(callee);
                                        // add_call_dependency(caller, callee);
                                        virtual_calls.push_back({ caller, callee });
                                        // continue;
                                    }
                                    // Add dependency to called vfunc
                                    
                                    // errs() << "Dependent vfuncs: " << '\n';
                                    for (std::string const & c : related_classes)
                                    {
                                        // std::string related_class_name = c.substr(std::string{"class."}.size());
                                        std::string related_class_name = c;
                                        auto it = vtables.find(related_class_name);
                                        if (it == vtables.end())
                                        {
                                            // errs() << "Not found: " << related_class_name << '\n';
                                            continue;
                                        }
                                        std::shared_ptr<ekstazi::VTable> vtable = it->second;
                                        // errs() << "Called class: " << vtable->get_name() << '\n';
                                        std::vector<Function*> const & derived_vfuns = vtable->get_vfuns();
                                        if (index > derived_vfuns.size() - 1)
                                        {
                                            // errs() << "VTable offset " << index << " is larger than vtable size: " << vfuns.size() << '\n';
                                            continue;
                                        }
                                        // errs() << "Related vfunc: " << derived_vfuns[index]->getName() << '\n';
                                        Function* related_callee = derived_vfuns[index];
                                        std::string callee_name = related_callee->getName();
                                        // Ignore pure virtual functions
                                        if (callee_name.find("__cxa_pure_virtual") == std::string::npos)
                                        {
                                            add_to_function_set(related_callee);
                                            // add_call_dependency(caller, related_callee);
                                            virtual_calls.push_back({ caller, related_callee });
                                        }
                                    }
                                }
                            }
                            // errs() << '\n';
                        }
                    }
                    
                    continue;
                }

                // Append to the set of functions
                // Make sure caller is not null and is in this module
                if (callee->isDeclaration())
                {
                    continue;
                }
                add_to_function_set(callee);
                add_call_dependency(caller, callee);
            }
        }


        // We didn't modify the module
        return false;
    }

    bool doFinalization(Module& M) override
    {
        timer.end();
        errs() << "Time for pass: " << timer.get_duration() << " ms\n";

        timer.start();

        // From global list of constructors, find the ones that are used by tests
        std::unordered_set<std::string> constructed_classes;
        for (auto & p : new_constructors)
        {
            errs() << "Constructor: " << p << '\n';
            std::unordered_set<std::string> dependents;
            std::unordered_set<std::string> dependents_old = old_depgraph.get_all_dependents(p);
            dependents.insert(dependents_old.begin(), dependents_old.end());
            std::unordered_set<std::string> dependents_new = new_depgraph.get_all_dependents(p);
            dependents.insert(dependents_new.begin(), dependents_new.end());
            // Search for tests
            for (auto & fun : dependents)
            {
                if (ekstazi::gtest::GtestAdapter::is_test_from_bc(fun))
                {
                    std::pair<std::string, std::string> class_fun_pair = ekstazi::Function::split_class_name(p);
                    constructed_classes.insert(class_fun_pair.first);
                }
            }
        }

        // Handle lazy-adding virtual calls here
        errs() << "Number of virtual calls: " << virtual_calls.size() << '\n';
        errs() << "Number of constructed classes: " << constructed_classes.size() << '\n';
        // for (auto & p : constructed_classes)
        // {
        //     errs() << "Constructed class: " << p << '\n';
        // }

        for (auto & p : virtual_calls)
        {
            Function* caller = p.first;
            Function* callee = p.second;
            if (caller == nullptr || caller->isDeclaration() ||
                callee == nullptr || callee->isDeclaration())
            {
                continue;
            }
            // errs() << "Virtual call: " << caller->getName() << ", " << callee->getName() << '\n';
            // Find class being invoked, and check if it was actually constructed in the code
            std::pair<std::string, std::string> class_fun_pair = ekstazi::Function::split_class_name(callee->getName());
            auto it = constructed_classes.find(class_fun_pair.first);
            if (it == constructed_classes.end())
            {
                // errs() << "Never constructed: " << class_fun_pair.first << '\n';
                continue;
            }
            add_call_dependency(caller, callee);
        }

        // Save the old and new metadata
        new_depgraph.save_file(new_depgraph_fname);
        old_depgraph.save_file(old_depgraph_fname);

        ekstazi::Function::save_file(new_functions, new_functions_fname);
        ekstazi::Function::save_file(old_functions, old_functions_fname);

        // Register all of the gtest tests
        gtest_adapter.register_tests(bc_fname, test_exec_fname);
        // gtest_adapter.register_tests(new_functions);

        errs() << "Finding modified functions..." << '\n';
        // Save the directly modified functions temporarily
        std::unordered_set<std::string> directly_modified_functions = ekstazi::Function::get_modified_functions(old_functions, new_functions);
        
        // errs() << "Directly Modified Functions: " << '\n';
        // for (std::string const & f : directly_modified_functions)
        // {
        //     errs() << f << '\n';
        // }

        // Now search through the graph to find connected nodes
        // Insert all traversed nodes into our set of modified functions
        errs() << "Looking for changes in dependency graph..." << '\n';
        for (std::string const & f : directly_modified_functions)
        {
            modified_functions.insert(f);
            std::unordered_set<std::string> dependents = old_depgraph.get_all_dependents(f);
            modified_functions.insert(dependents.begin(), dependents.end());

            dependents = new_depgraph.get_all_dependents(f);
            modified_functions.insert(dependents.begin(), dependents.end());
        }
        // errs() << "Propagated Changes: " << '\n';
        // for (std::string const & f : modified_functions)
        // {
        //     errs() << f << '\n';
        // }
        std::ofstream ofs{ modified_functions_fname };
        for (std::string const & f : modified_functions)
        {
            ofs << f << std::endl;
        }
        ofs.close();

        // Now we need to filter out the test functions.
        modified_tests = gtest_adapter.get_modified_filters(modified_functions);

        errs() << "Modified Test Size: " << modified_tests.size() << '\n';

        // errs() << "Modified Tests: " << '\n';
        ofs = std::ofstream{ modified_tests_fname };
        for (std::string const & t : modified_tests)
        {
            ofs << t << std::endl;
            // errs() << t << '\n';
        }
        ofs.close();

        timer.end();
        errs() << "Time for Finalization: " << timer.get_duration() << " ms\n";
        
        return false;
    }

protected:
    std::string compute_checksum(Function* f)
    {
        FunctionComparator::FunctionHash hash = ekstazi::FunctionComparator::functionHash(*f);
        // errs() << "Finished computing checksum: " << hash << '\n';
        return std::to_string(hash);
    }

    void build_class_hierarchy(Module & module)
    {
        for (GlobalVariable & gv : module.globals())
        {
            if (!ekstazi::VTable::is_vtable_def(gv))
            {
                continue;
            }
            // Get type of virtual table to build inheritance hierarchy
            SmallVector<MDNode*, 8> mds;
            gv.getMetadata("type", mds);
            if (mds.size() > 1)
            {
                // VTable type is the last element of metadata vector
                MDNode* vt_type = mds[mds.size() - 1];
                // errs() << *vt_type << '\n';
                MDString* vt_type_md_str = dyn_cast<MDString>(vt_type->getOperand(1));
                if (vt_type_md_str == nullptr)
                {
                    continue;
                }
                std::string type_name = ekstazi::demangle(vt_type_md_str->getString());
                // errs() << type_name << ',';
                type_name = type_name.substr(std::string("typeinfo name for ").size());

                // Add type relationships from type to all of its previous elements in MD vector
                for (int i = 0; i < mds.size() - 1; ++i)
                {
                    MDOperand const & super_type_md = mds[i]->getOperand(1);
                    MDString* super_type_md_str = dyn_cast<MDString>(super_type_md);
                    if (super_type_md_str == nullptr)
                    {
                        continue;
                    }
                    std::string super_type_name = ekstazi::demangle(super_type_md_str->getString());
                    super_type_name = super_type_name.substr(std::string("typeinfo name for ").size());
                    new_type_hierarchy.add_inheritance_relationship(super_type_name, type_name);
                }


            }
        }

        new_type_hierarchy.save_file(new_type_hierarchy_fname);
    }

    /**
     * Build the vtables for the module.
     */
    void build_vtables(Module & module)
    {
        for (GlobalVariable & gv : module.globals())
        {
            if (ekstazi::VTable::is_vtable_def(gv))
            {
                std::shared_ptr<ekstazi::VTable> vtable = std::make_shared<ekstazi::VTable>();
                vtable->add_entries(&gv);
                vtables.insert({ vtable->get_name(), vtable });
                // vtables.add_vtable(&gv);
            }
        }
        errs() << "Number of virtual tables found: " << vtables.size() << '\n';

        // for (auto & p : vtables)
        // {
        //     errs() << p.first << '\n';
        //     for (auto & s : p.second->get_vfuns())
        //     {
        //         errs() << s << '\n';
        //     }
        // }
    }

    /**
     * Extracts the containing class from the function if it exists.
     * Format of a LLVM function:
     * {namespaces...}::{className}::{functionName}
     */
    std::string get_containing_class(std::string const & fun_name)
    {
        if (fun_name.find("::") == std::string::npos)
        {
            return "";
        }
        // Find where the params start
        size_t params_start_pos = fun_name.find('(');

        // Now find the first '::'
        size_t fun_name_pos = fun_name.rfind("::", params_start_pos);

        // Find the second '::'
        size_t class_name_pos = fun_name.rfind("::", fun_name_pos);

        if (class_name_pos == std::string::npos)
        {
            return fun_name.substr(0, fun_name_pos);
        }

        return fun_name.substr(class_name_pos + 2, fun_name_pos - class_name_pos);
    }

    /**
     * Adds a function to the ekstazi function set.
     * 
     * @param {fun} the function to be added. Must be a valid function (non-null, not a declaration).
     */
    void add_to_function_set(Function* fun)
    {
        if (fun == nullptr || fun->isDeclaration())
        {
            return;
        }
        std::string const & fun_name = ekstazi::demangle(fun->getName().str());
        std::string const & fun_fname = fun->getParent()->getSourceFileName();
        std::string fun_checksum = compute_checksum(fun);
        ekstazi::Function fun_ekstazi{ fun_name, fun_fname, fun_checksum };
        new_functions.insert({ fun_ekstazi.name(), fun_ekstazi });

        // If the function is a constructor, add it to the constructor set
        if (ekstazi::Function::is_constructor(fun->getName()))
        {
            // std::pair<std::string, std::string> p = ekstazi::Function::split_class_name(fun->getName());
            new_constructors.insert(fun_ekstazi.name());
            // new_constructors.insert({ fun_ekstazi.name(), fun_ekstazi });
        }
    }

    /**
     * Adds a dependency to the ekstazi dependency graph.
     * 
     * @param {caller} The caller function.
     * @param {callee} The callee function.
     * @param {is_virtual} Whether or not the call is virtual.
     */
    void add_call_dependency(Function* caller, Function* callee, bool is_virtual=false)
    {
        // Only handle functions in this module
        if (caller->isDeclaration() || callee->isDeclaration())
        {
            return;
        }

        // Don't add internal gtest functions
        if (ekstazi::gtest::GtestAdapter::is_internal_function(caller->getName()) ||
            ekstazi::gtest::GtestAdapter::is_internal_function(callee->getName()))
        {
            return;
        }

        // Add the dependency to the dependency graph
        std::string const caller_name = ekstazi::demangle(caller->getName().str());
        std::string const callee_name = ekstazi::demangle(callee->getName().str());

        // if (caller_name.find("TestBody") != std::string::npos)
        //     errs() << caller_name << ", " << callee_name << '\n';
        
        new_depgraph.add_dependency(callee_name, caller_name);
    }
    
}; // end of struct Filename
}  // end of anonymous namespace

char Ekstazi::ID = 0;
static RegisterPass<Ekstazi> X("ekstazi", "Ekstazi Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
