
#include "ekstazi/test-frameworks/gtest/gtest-adapter.hh"

#include "ekstazi/test-frameworks/gtest/test-types/value-parameterized-test.hh"
#include "ekstazi/test-frameworks/gtest/test-types/typed-test.hh"
#include "ekstazi/test-frameworks/gtest/test-types/type-parameterized-test.hh"
#include "ekstazi/utils/mangle.hh"

#include <sstream>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <pstreams/pstream.h>
#include <vector>

namespace ekstazi
{

namespace gtest
{

std::string const GtestAdapter::bc_suffix = ".0.5.precodegen.bc";

/**
 * Returns whether or not a function name in IR is an internal
 * googletest function. We will skip adding these to the dependency
 * graph and function set.
 * 
 * @param fun_name the mangled function name
 */
bool GtestAdapter::is_internal_function(std::string const & fun_name)
{
    std::string demangled_name = demangle(fun_name);
    return
        // default testing:: namespace
        // demangled_name.find("testing::") == 0 ||

        // testing::internal functions
        demangled_name.find("testing::internal") != std::string::npos ||
        demangled_name.find("testing::Assertion") == 0 ||
        demangled_name.find("testing::Message") == 0 ||
        demangled_name.find("testing::Test") == 0 ||
        demangled_name.find("testing::UnitTest") == 0;
        // (fun_name.find("testing") != std::string::npos && fun_name.find("internal") != std::string::npos) ||
        // (fun_name.find("testing") != std::string::npos && fun_name.find("Assertion") != std::string::npos) ||
        // fun_name.find("test_info") != std::string::npos;
}

/**
 * Returns whether or not a function name in IR is a
 * googletest test.
 */
bool GtestAdapter::is_test_from_bc(std::string const & fun_name)
{
    return
        ValueParameterizedTest::is_test_from_bc(fun_name) ||
        TypeParameterizedTest::is_test_from_bc(fun_name) ||
        TypedTest::is_test_from_bc(fun_name) ||
        Test::is_test_from_bc(fun_name);
}


GtestAdapter::GtestAdapter()
{

}

/**
 * Registers a single test given the name of its function.
 * 
 * @return true if the given function was a test, false if not.
 */
bool GtestAdapter::register_test(std::string const & fun_name)
{
    if (ValueParameterizedTest::is_test_from_bc(fun_name))
    {
        // std::cout << "Value-Param Test: " << fun_name << std::endl;
        bc_tests.insert({ fun_name, std::make_shared<ValueParameterizedTest>(fun_name) });
        return true;
    }

    if (TypeParameterizedTest::is_test_from_bc(fun_name))
    {
        // std::cout << "Type-Param Test: " << fun_name << std::endl;
        bc_tests.insert({ fun_name, std::make_shared<TypeParameterizedTest>(fun_name) });
        return true;
    }

    if (TypedTest::is_test_from_bc(fun_name))
    {
        // std::cout << "Typed Test: " << fun_name << std::endl;
        bc_tests.insert({ fun_name, std::make_shared<TypedTest>(fun_name) });
        return true;
    }

    if (Test::is_test_from_bc(fun_name))
    {
        // std::cout << "Normal test: " << fun_name << std::endl;
        bc_tests.insert({ fun_name, std::make_shared<Test>(fun_name) });
        return true;
    }

    return false;
}

/**
 * Registers all gtest tests given the list of all functions in the module.
 * We first register all special tests (parameterized) and then normal tests.
 * 
 * The executable should be generated right next to the bitcode file.
 */
void GtestAdapter::register_tests(std::string const & module_name, std::string const & opt_executable_name)
{
    std::string executable_name = opt_executable_name;
    // If not given the input executable, assume it's the same as the bitcode
    if (executable_name.empty())
    {
        // Strip the suffix from the module name
        size_t pos_suffix = module_name.find(bc_suffix);
        executable_name = module_name.substr(0, pos_suffix);
    }
    
    std::cout << "Test exec name: " << executable_name << std::endl;

    // Make sure this is a gtest executable
    redi::ipstream proc{ { executable_name, "--help" }, redi::pstreams::pstdout | redi::pstreams::pstderr };
    std::string line;
    bool is_gtest_exec = false;
    while (std::getline(proc, line))
    {
        if (line.find("--gtest") != std::string::npos)
        {
            is_gtest_exec = true;
            break;
        }
    }
    proc.close();
    if (!is_gtest_exec)
    {
        std::cout << "Not a gtest executable: " << executable_name << std::endl;
        exit(1);
    }

    // Now list the tests and put into a stringstream
    redi::ipstream proc_list_tests{ { executable_name, "--gtest_list_tests" }, redi::pstreams::pstdout | redi::pstreams::pstderr };

    std::stringstream ss;
    while (std::getline(proc_list_tests, line))
    {
        // std::cout << line << std::endl;
        ss << line << std::endl;
    }
    proc_list_tests.close();

    create_tests_from_exec(ss);

    // for (auto const & pair : tests)
    // {
    //     std::cout << pair.second->get_map_key() << pair.second->get_map_key().size() << std::endl;
    // }
}

/**
 * Returns the list of modified gtest filters given the set of
 * modified functions.  Before actually beginning processing, we need
 * to remove all value-parameterized tests from the map of tests.
 */
std::unordered_set<std::string> GtestAdapter::get_modified_filters(std::unordered_set<std::string> const & modified_funs)
{
    std::unordered_set<std::string> results;
    for (std::string const & modified_fun : modified_funs)
    {
        if (TypeParameterizedTest::is_test_from_bc(modified_fun))
        {
            TypeParameterizedTest test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            // std::cout << "Type param test: " << test.get_test_case_name() + '_' + test.get_test_name() << std::endl;
            results.insert(test_exec->get_gtest_filter_name());
        }

        else if (TypedTest::is_test_from_bc(modified_fun))
        {
            TypedTest test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            // std::cout << "Type param test: " << test.get_test_case_name() + '_' + test.get_test_name() << std::endl;
            results.insert(test_exec->get_gtest_filter_name());
        }

        // In bc, tests look exactly like parameterized tests
        else if (Test::is_test_from_bc(modified_fun))
        {
            Test test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            // std::cout << "Test: " << test.get_test_case_name() + '_' + test.get_test_name() << std::endl;
            // std::cout << test_exec->get_gtest_filter_name() << std::endl;
            // std::cout << "here?" << std::endl;
            results.insert(test_exec->get_gtest_filter_name());
        }
    }    

    return results;
}

/**
 * Returns the modified tests.
 */
std::unordered_map<std::string, std::shared_ptr<Test>> GtestAdapter::get_modified_tests(std::unordered_set<std::string> const & modified_funs)
{
    std::unordered_map<std::string, std::shared_ptr<Test>> results;
    for (std::string const & modified_fun : modified_funs)
    {
        if (TypeParameterizedTest::is_test_from_bc(modified_fun))
        {
            TypeParameterizedTest test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            results[test_exec->get_map_key()] = test_exec;
        }

        else if (TypedTest::is_test_from_bc(modified_fun))
        {
            TypedTest test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            results[test_exec->get_map_key()] = test_exec;
        }

        // In bc, tests look exactly like parameterized tests
        else if (Test::is_test_from_bc(modified_fun))
        {
            Test test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            results[test_exec->get_map_key()] = test_exec;
        }
    }    

    return results;
}

/**
 * Returns the modified tests when selecting by case.
 */
std::unordered_map<std::string, std::shared_ptr<Test>> GtestAdapter::get_modified_tests_sel_case(std::unordered_set<std::string> const & modified_funs)
{
    std::unordered_map<std::string, std::shared_ptr<Test>> results;
    for (std::string const & modified_fun : modified_funs)
    {
        if (TypeParameterizedTest::is_test_from_bc(modified_fun))
        {
            TypeParameterizedTest test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            results[test_exec->get_map_key()] = test_exec;

            // Now add all tests that match the case
            for (auto & p : tests)
            {
                if (test_exec->get_test_case_name() == p.second->get_test_case_name())
                {
                    results[p.second->get_map_key()] = p.second;
                }
            }
        }

        else if (TypedTest::is_test_from_bc(modified_fun))
        {
            TypedTest test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            // std::cout << "Type param test: " << test.get_test_case_name() + '_' + test.get_test_name() << std::endl;
            results[test_exec->get_map_key()] = test_exec;

            // Now add all tests that match the case
            for (auto & p : tests)
            {
                if (test_exec->get_test_case_name() == p.second->get_test_case_name())
                {
                    results[p.second->get_map_key()] = p.second;
                }
            }
        }

        // In bc, tests look exactly like parameterized tests
        else if (Test::is_test_from_bc(modified_fun))
        {
            Test test{ modified_fun };
            std::unordered_map<std::string, std::shared_ptr<Test>>::iterator it = tests.find(test.get_map_key());
            // std::cout << "Key: " << test.get_map_key() << std::endl;
            if (it == tests.end())
            {
                std::cout << "Not found: " << test.get_map_key() << test.get_map_key().size() << std::endl;
                continue;
            }
            std::shared_ptr<Test> test_exec = it->second;
            results[test_exec->get_map_key()] = test_exec;

            // Now add all tests that match the case
            for (auto & p : tests)
            {
                if (test_exec->get_test_case_name() == p.second->get_test_case_name())
                {
                    results[p.second->get_map_key()] = p.second;
                }
            }
        }
    }    

    return results;
}



std::string get_gtest_filter()
{
    // First check if ekstazi has been run yet
    std::string count_fname = ".ekstazi/count.ekstazi";
    std::ifstream ifs{ count_fname };
    std::string line;
    std::getline(ifs, line);
    std::stringstream ss{ line };
    int count;
    ss >> count;

    // First time, so return empty filter
    if (count == 1)
    {
        ifs.close();
        return "*";
    }

    ifs.close();

    std::string fname = ".ekstazi/modified-tests.txt";

    std::string gtest_filter{};

    ifs = std::ifstream{ fname };
    bool first = true;
    while (std::getline(ifs, line))
    {
        // Remove namespaces from the test name
        size_t index = line.find_last_of("::");
        if (index != std::string::npos)
        {
            line = line.substr(index + 1);
        }

        if (first)
        {
            gtest_filter += line;
            first = false;
            continue;
        }
        gtest_filter += ":" + line;
    }

    std::cout << "Gtest filter is: " << gtest_filter << std::endl;

    return gtest_filter;
}

/**
 * Remaps all value-parameterized tests to their TestBody()
 * equivalents.  Since value-parameterized tests are equivalent to
 * normal tests in bitcode, we first register value-parameterized
 * tests using a different function signature. Thus, we need to remap
 * all registered parameterized tests to their ::TestBody()
 * equivalents.
 */
void GtestAdapter::remap_value_parameterized_tests()
{
    // Search through all value-parameterized tests.
    for (std::pair<std::string, std::shared_ptr<Test>> const & p : bc_tests)
    {
        // if (p.second)
        std::string const & fun_name = p.first;
        if (ValueParameterizedTest::is_test_from_bc(fun_name))
        {
            std::string testbody_fun_name = p.second->get_testbody_fun_name();
            // std::cout << "Testbody: " << testbody_fun_name << std::endl;
            auto find = bc_tests.find(testbody_fun_name);
            if (find == bc_tests.end())
            {
                std::cout << "Error: Value-Parameterized test not found: " << testbody_fun_name << std::endl;
            }
            bc_tests[testbody_fun_name] = p.second;
        }
    }
}

/**
 * Creates all test cases from the output of a test executable when
 * run with --gtest_list_tests.
 * 
 * @param {proc} the test executable's process stream
 */
void GtestAdapter::create_tests_from_exec(std::stringstream & proc)
{
    std::string line;
    uint32_t line_num = 0;
    while (std::getline(proc, line))
    {
        // Check if the first line is valid or not.
        if (line.find("Running main() from gtest_main.cc") != std::string::npos)
        {
            continue;
        }
        // if (line_num < 1)
        // {
        //     ++line_num;
        //     continue;
        // }
        std::string test_case_name{ line };
        // std::cout << "Test case: " << test_case_name << std::endl;

        create_tests_for_test_case(test_case_name, proc);

        // Now we check for the string "# TypeParam = ${type}", which indicates a typed test
        
        ++line_num;
    }
}

/**
 * Determines the test type given the test case name. This function
 * then calls the template-overloaded function
 * create_tests_for_test_case<> with the correct test type.
 */
void GtestAdapter::create_tests_for_test_case(std::string const & test_case_name, std::stringstream & proc)
{
    // std::cout << "Finding tests for " << test_case_name << std::endl;
    std::string line;
    // We need to peek at the next line to get the first test name.
    redi::ipstream::pos_type pos = proc.tellg();
    if (!std::getline(proc, line))
    {
        std::cout << "Unexpected end of input" << std::endl;
        throw "Unexpected end of input";
    }
    std::string first_test_name{ line };
    // Rewind back one line
    proc.seekg(pos);

    if (test_case_name.find('.') == std::string::npos || first_test_name[0] != ' ')
    {
        std::cout << "Not a test: " << test_case_name << std::endl;
        return;
    }

    // std::cout << "TestCase, TestName: " << test_case_name << ", " << first_test_name << std::endl;
    // Order doesn't really matter here.
    if (TypeParameterizedTest::is_test_from_exec(test_case_name, first_test_name))
    {
        create_tests_for_test_case<TypeParameterizedTest>(test_case_name, proc);
        return;
    }

    if (TypedTest::is_test_from_exec(test_case_name, first_test_name))
    {
        create_tests_for_test_case<TypedTest>(test_case_name, proc);
        return;
    }
    if (ValueParameterizedTest::is_test_from_exec(test_case_name, first_test_name))
    {
        create_tests_for_test_case<ValueParameterizedTest>(test_case_name, proc);
        return;
    }
    
    if (Test::is_test_from_exec(test_case_name, first_test_name))
    {
        create_tests_for_test_case<Test>(test_case_name, proc);
        return;
    }

    return;
}

/**
 * Creates test cases from a test case. This function is called
 * internally by create_tests_from_exec to parse the tests after
 * determing the test case.
 */
template <typename T>
void GtestAdapter::create_tests_for_test_case(std::string const & test_case_name, std::stringstream & proc)
{
    std::string line;
    // Assert that T is of type Test
    static_assert(std::is_base_of<Test, T>::value, "Must derive from Test");
    while (std::getline(proc, line))
    {
        std::string test_name{ line };

        std::shared_ptr<Test> test = std::make_shared<T>(test_case_name, test_name);
        std::string test_map_key = test->get_map_key();
        tests[test_map_key] = test;

        // Break the loop if we're at the next test case.
        // We can check for the next case by seeing if next != ' '
        char next = proc.peek();
        if (next != ' ')
        {
            break;
        }
    }
}

}


}
