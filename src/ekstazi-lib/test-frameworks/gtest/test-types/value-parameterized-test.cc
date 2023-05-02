
#include "ekstazi/test-frameworks/gtest/test-types/value-parameterized-test.hh"

#include <iostream>
namespace ekstazi
{

namespace gtest
{

/**
 * In bitcode, value-parameterized tests look just like normal gtest
 * tests if we only look for ::TestBody().  However, we can
 * differentiate value-parameterized tests by looking for the
 * following signature:
 * 
 * testing::internal::ParameterizedTestFactory<{namespace}::{TestCase}_{TestName}_Test>::CreateTest()
 * 
 * Thus, we need to register that this is a value-parameterized test
 * when first parsing through the bitcode.
 */
// We look for this to see if a test name is this test type
std::string const ValueParameterizedTest::test_name_signature{ "# GetParam() = " };

std::string const ValueParameterizedTest::signature_begin = "testing::internal::ParameterizedTestFactory";
std::string const ValueParameterizedTest::signature_end{ "_Test>::CreateTest()" };

bool ValueParameterizedTest::is_test_from_bc(std::string const & fun_name)
{
    // We need to search for both the beginning and end signature
    size_t pos = fun_name.find(signature_begin);
    if (pos == std::string::npos)
    {
        return false;
    }

    pos = fun_name.find(signature_end, fun_name.size() - signature_end.size());
    if (pos == std::string::npos)
    {
        return false;
    }

    return true;
}

/**
 * Returns whether the given test case indicates that the test case is
 * a typed test.  Typed tests look like the following in
 * --gtest_list_tests:
 * 
 * {Prefix}/{TestCase}.
 *   {Test1Name}/0  # GetParam() = {Param1}
 *   {Test1Name}/1  # GetParam() = {Param2}
 */
bool ValueParameterizedTest::is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name)
{
    return first_test_name.find(test_name_signature) != std::string::npos;
}

ValueParameterizedTest::ValueParameterizedTest(std::string const & fun_name) :
Test::Test{ fun_name }
{
    parse_fun_name();
}

ValueParameterizedTest::ValueParameterizedTest(std::string const & test_case_name, std::string const & test_name)
{
    // There should only be one optional delimiter between the prefix
    // and test case
    size_t pos_prefix = test_case_name.find(gtest_prefix_delim);
    if (pos_prefix != std::string::npos)
    {
        prefix = test_case_name.substr(0, pos_prefix);
        prefix = strip_whitespace(prefix);
    }
    else
    {
        prefix = "";
    }
    
    this->test_case_name = test_case_name.substr(pos_prefix + 1);
    this->test_case_name = strip_whitespace(this->test_case_name);
    // Remove the trailing '.'
    this->test_case_name.erase(this->test_case_name.size() - 1);

    // Now we search for the suffix '/' for the param
    size_t pos_suffix = test_name.find(gtest_prefix_delim);
    this->test_name = test_name.substr(0, pos_suffix);
    this->test_name = strip_whitespace(this->test_name);
    
    // Note that we now search through the test name for the param
    size_t pos_param = test_name.find(test_name_signature);
    std::string param_index_str = test_name.substr(pos_suffix + 1, pos_param - pos_suffix);
    param_index_str = strip_whitespace(param_index_str);
    // param_index = std::stoi(param_index_str);
    param_index = param_index_str;
    
    param = test_name.substr(pos_param + test_name_signature.size());
    param = strip_whitespace(param);

    // std::cout << prefix << '_' << this->test_case_name << '_' << this->test_name << '_' << param_index << '_' << param << std::endl;
}

std::string ValueParameterizedTest::get_testbody_fun_name()
{
    parse_fun_name();
    std::string result{ namespace_name + "::" + test_case_name + '_' + test_name + "_Test::TestBody()" };
    return result;
}

std::string ValueParameterizedTest::get_gtest_filter_name()
{
    // parse_fun_name();
    return "*" + test_case_name + "." + test_name + "*";
}

// void ValueParameterizedTest::parse_fun_name()
// {
//     // First keep track of where the TestCase begins
//     size_t pos_test_case = signature_begin.size() + 1;

//     // Find the location of the character right after the first underscore, which is the TestCase_TestName separator
//     size_t pos_test_name = fun_name.find_first_of('_', pos_test_case) + 1;

//     // Find the location of the character right after the second underscore, which is the TestName_Test separator
//     size_t pos_end = fun_name.find_first_of('_', pos_test_name) + 1;

//     test_case_name = fun_name.substr(pos_test_case, pos_test_name - pos_test_case - 1);
//     // Remember to strip the namespace from the beginning of the test case
//     test_case_name = strip_namespace(test_case_name);
//     // std::cout << "Test Case: " << test_case_name << std::endl;

//     test_name = fun_name.substr(pos_test_name, pos_end - pos_test_name - 1);
//     // std::cout << "Test Name: " << test_name << std::endl;
// }

}

}
