#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <vector>
#include <pstreams/pstream.h>

#include "ekstazi/test-frameworks/gtest/test-types/test.hh"
#include "ekstazi/depgraph/function.hh"

namespace ekstazi
{

namespace gtest
{

class GtestAdapter
{
public:
    static const std::string bc_suffix;

    /**
     * Returns whether or not a function name in IR is an internal
     * googletest function. We will skip adding these to the dependency
     * graph and function set.
     * 
     * @param fun_name the mangled function name
     */
    static bool is_internal_function(std::string const & fun_name);

    /**
     * Returns whether or not a function name in IR is a
     * googletest test.
     */
    static bool is_test_from_bc(std::string const & fun_name);    
    
    GtestAdapter();

    /**
     * Registers a single test given the name of its function.
     * 
     * @return true if the given function was a test, false if not.
     */
    bool register_test(std::string const & fun);

    /**
     * Registers all gtest tests given the list of all functions in the module.
     */
    void register_tests(std::string const & module_name, std::string const & opt_executable_name = "");

    /**
     * Returns the list of modified gtest filters given the set of modified functions.
     */
    std::unordered_set<std::string> get_modified_filters(std::unordered_set<std::string> const & modified_funs);

    /**
     * Returns the modified tests.
     */
    std::unordered_map<std::string, std::shared_ptr<Test>> get_modified_tests(std::unordered_set<std::string> const & modified_funs);

    /**
     * Returns the modified tests when selecting by case.
     */
    std::unordered_map<std::string, std::shared_ptr<Test>> get_modified_tests_sel_case(std::unordered_set<std::string> const & modified_funs);

protected:
    /**
     * Remaps all value-parameterized tests to their TestBody() equivalents.
     * Since value-parameterized tests are equivalent to normal tests in bitcode, we first register
     * value-parameterized tests using a different function signature. Thus, we need to remap
     * all registered parameterized tests to their ::TestBody() equivalents.
     */
    void remap_value_parameterized_tests();

    /**
     * Creates all test cases from the output of a test executable when run with
     * --gtest_list_tests.
     * 
     * @param {proc} the test executable's process stream
     */
    void create_tests_from_exec(std::stringstream & proc);

    /**
     * Determines the test type given the test case name. This function then calls
     * the template-overloaded function create_tests_for_test_case<> with the correct test type.
     */
    void create_tests_for_test_case(std::string const & test_case_name, std::stringstream & proc);

    /**
     * Creates test cases from a test case. This function is called internally by
     * the non-template function create_test_for_test_case to parse the tests after determing the test case.
     */
    template <typename T>
    void create_tests_for_test_case(std::string const & test_case_name, std::stringstream & proc);

    std::unordered_map<std::string, std::shared_ptr<Test>> tests;
    std::unordered_map<std::string, std::shared_ptr<Test>> bc_tests;
};

/**
 * This function takes a set of tests and filters out the Google Test tests.
 */
std::unordered_set<std::string> filter_tests(std::unordered_set<std::string> const & funs);

/**
 * Reads the current ekstazi metadata and returns the appropriate gtest filter string.
 */
std::string get_gtest_filter();

}

}
