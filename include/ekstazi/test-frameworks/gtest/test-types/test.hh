#pragma once

#include <string>

#include "ekstazi/test-frameworks/gtest/test-types/test.hh"

namespace ekstazi
{

namespace gtest
{

/**
 * The Test class represents any gtest that is instantiated.
 * Tests look like the following:
 * 
 * {namespace}::{TestCase}_{TestName}_Test::TestBody()
 */
class Test
{
public:
    /**
     * Maximum length of any parameter (type or value)
     */
    static const uint32_t max_param_length;

    // Delimiter between the test prefix and test case.
    static char const gtest_prefix_delim;

    // We look for this to see if a test case is this test type
    static std::string const test_case_name_signature;

    // We look for this to see if a test name is this test type
    static std::string const test_name_signature;
    
    static std::string const signature;
    /**
     * Returns whether the given string is a gtest test given the full function name.
     */
    static bool is_test_from_bc(std::string const & fun_name, std::string const & signature = Test::signature);

    /**
     * Returns whether the given test case indicates that the test case is a normal test.
     * Tests look like the following in --gtest_list_tests:
     * 
     * {TestCase}.
     *   {Test1Name}
     *   {Test2Name}
     */
    static bool is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name);

    Test();
    
    Test(std::string const & fun_name);

    Test(std::string const & test_case_name, std::string const & test_name);

    /**
     * Returns the gtest filter representing the given test name.
     */
    virtual std::string get_gtest_filter_name();

    /**
     * Returns the name of the test body function name. Since most tests look for TestBody() as their signature,
     * this will return the function name in most of the cases. However, for the Value-Parameterized Test, we
     * will need to actually parse the TestCase and TestName and return the TestBody() equivalent.
     */
    virtual std::string get_testbody_fun_name();

    virtual std::string get_test_case_name() const;
    virtual std::string get_test_name() const;

    /**
     * Returns the map key string that corresponds to this test. The map key is suitable to be used in a hash map
     * to be a unique identifier for this test. Make sure that the map key can be constructed from both the
     * test executable (gtest_list_tests) as well as the bitcode representation.
     */
    virtual std::string get_map_key();
    

    virtual ~Test();


protected:
    /**
     * Strips the leading and trailing whitespace from a string.
     */
    static std::string strip_whitespace(std::string const & str);

    /**
     * Strips the preceding namespaces from a string.
     * e.g. ekstazi::TestCase -> TestCase
     */
    std::string strip_namespace(std::string const & str);

    /**
     * Parses the fun_name and sets the test_case_name and test_name fields.
     * Note that for a normal test, we cannot differentiate between the test case and
     * test name if they contain an underscore. Thus, we just assume the first underscore
     * splits the test case and test name, while the last underscore splits the test name and
     * the "test" keyword. This way, when we concatenate the test case and test name, it will
     * exist in our tests map.
     */
    virtual void parse_fun_name();

    std::string fun_name;

    std::string namespace_name;
    std::string test_case_name;
    std::string test_name;
};

}

}
