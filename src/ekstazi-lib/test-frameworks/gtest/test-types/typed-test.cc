
#include "ekstazi/test-frameworks/gtest/test-types/typed-test.hh"

#include <iostream>

namespace ekstazi
{

namespace gtest
{

/**
 * We can tell what is a typed test by analyzing the signature for a
 * normal gtest.  The typed test will be different in that it contains
 * a type:
 * 
 * {namespace}::{TestCase}_{TestName}_Test<{TestType}>::TestBody().
 */
std::string const TypedTest::signature_begin{ "_Test<" };
std::string const TypedTest::signature_end{ ">::TestBody()" };

std::string const TypedTest::test_case_name_signature = "# TypeParam = ";

bool TypedTest::is_test_from_bc(std::string const & fun_name)
{
    return fun_name.find(signature_begin) != std::string::npos && fun_name.find(signature_end) != std::string::npos;
}

/**
 * Returns whether the given test case indicates that the test case is
 * a typed test.  Typed tests look like the following in
 * --gtest_list_tests:
 * 
 * {TestCase}/{TypeParamIndex}.  # TypeParam = {Type}
 *   {Test1Name}
 *   {Test2Name}
 */
bool TypedTest::is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name)
{
    // There should only be one delimiter since there is no prefix.
    size_t pos_delim = test_case_name.find(gtest_prefix_delim);
    if (pos_delim == std::string::npos)
    {
        return false;
    }

    // We return false if we find another delimiter here.
    pos_delim = test_case_name.find(gtest_prefix_delim, pos_delim + 1);
    if (pos_delim != std::string::npos)
    {
        return false;
    }

    // Now make sure the test case contains the signature.
    return test_case_name.find(test_case_name_signature) != std::string::npos;
}

TypedTest::TypedTest(std::string const & fun_name)
// Test::Test{ fun_name }
{
    this->fun_name = fun_name;
    parse_fun_name();
}

TypedTest::TypedTest(std::string const & test_case_name, std::string const & test_name)
{
    // Split the test case and the type param index by '/'
    size_t pos_suffix = test_case_name.find(gtest_prefix_delim);
    this->test_case_name = test_case_name.substr(0, pos_suffix);
    
    // We need to split the test case into the TestCase and Type.
    size_t pos_type = test_case_name.find(test_case_name_signature, pos_suffix + 1);
    std::string param_index_str = test_case_name.substr(pos_suffix + 1, pos_type - pos_suffix - 1);
    param_index_str = strip_whitespace(param_index_str);
    // Remove the trailing '.'
    param_index_str.erase(param_index_str.size() - 1);
    type_param_index = std::stoi(param_index_str);

    // Now find the type param
    type_param = test_case_name.substr(pos_type + test_case_name_signature.size());
    type_param = strip_whitespace(type_param);

    // Check if the type needs to be truncated
    if (type_param.size() > max_param_length)
    {
        type_param = type_param.substr(0, max_param_length);
    }

    // Remove the trailing '.'
    this->test_name = strip_whitespace(test_name);

    // std::cout << this->test_case_name << '_' << this->test_name << '_' << type_param_index << '_' << type_param << std::endl;
}


std::string TypedTest::get_gtest_filter_name()
{
    // parse_fun_name();
    // Note the '*' here matches all {TestCase}*.{TestName}, handling all suffixes
    return test_case_name + '/' + std::to_string(type_param_index) + '.' + test_name;
}

/**
 * Returns the map key string that corresponds to this test. The map
 * key is suitable to be used in a hash map to be a unique identifier
 * for this test. Make sure that the map key can be constructed from
 * both the test executable (gtest_list_tests) as well as the bitcode
 * representation.
 * 
 * For typed tests, the map key will be {TestCase}_{TestName}_{TypeParam}
 */
std::string TypedTest::get_map_key()
{
    return test_case_name + '_' + test_name + '_' + type_param;
}

/**
 * Parses the fun_name and sets the test_case_name and test_name fields.
 */
void TypedTest::parse_fun_name()
{
    // Remove the signature and namespace from the function
    // Remove >::TestBody()
    size_t pos_signature = fun_name.rfind(signature_end);
    // Don't remove the last '>'
    fun_name = fun_name.substr(0, pos_signature + 1);

    // Find the location of the first underscore, which is the
    // TestCase_TestName separator
    size_t pos_test_case_delim = fun_name.find('_');

    // Find '_Test<'
    size_t pos_test_name_end = fun_name.rfind(signature_begin);

    // Find the type param delimiters
    size_t pos_type_start = fun_name.find('<', pos_test_name_end);
    size_t pos_type_end = fun_name.rfind('>');

    type_param = fun_name.substr(pos_type_start + 1, pos_type_end - pos_type_start - 1);
    type_param = strip_whitespace(type_param);
    if (type_param.size() > max_param_length)
    {
        type_param = type_param.substr(0, max_param_length);
    }

    // Don't strip the '<' after _Test<
    fun_name = fun_name.substr(0, pos_type_start + 1);
    
    fun_name = strip_namespace(fun_name);

    // Re-search positions after stripping namespace
    pos_test_case_delim = fun_name.find('_');
    pos_test_name_end = fun_name.rfind(signature_begin);

    test_case_name = fun_name.substr(0, pos_test_case_delim);
    // Remember to strip the namespace from the beginning of the test case
    // test_case_name = strip_namespace(test_case_name);
    // std::cout << "Test Case: " << test_case_name << std::endl;

    test_name = fun_name.substr(pos_test_case_delim + 1, pos_test_name_end - pos_test_case_delim - 1);

    // std::cout << "testcase, testname, type: " << test_case_name << ',' << test_name << ',' << type_param << std::endl;
}

}

}
