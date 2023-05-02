
#include "ekstazi/test-frameworks/gtest/test-types/type-parameterized-test.hh"
#include <iostream>

namespace ekstazi
{

namespace gtest
{

/**
 * Type-Parameterized tests look like this in bitcode:
 * 
 * {namespace}::gtest_case_{TestCase}_::{TestName}<char>::TestBody()
 * 
 * Thus, we just need to look for the beginning and end signatures.
 */

std::string const TypeParameterizedTest::test_case_name_signature{ "# TypeParam = " };
// Delimiter between the test prefix and test case.
// char const Test::gtest_prefix_delim = '/';

std::string const TypeParameterizedTest::signature_begin{ "gtest_case_" };
std::string const TypeParameterizedTest::signature_end{ "::TestBody()" };

bool TypeParameterizedTest::is_test_from_bc(std::string const & fun_name)
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
    // std::cout << "Type-Param: " << fun_name << std::endl;

    return true;
}

/**
 * Returns whether the given test case indicates that the test case is
 * a type-parameterized test.  Type-parameterized tests look like the
 * following in --gtest_list_tests:
 * 
 * {Prefix}/{TestCase}/{TypeParamIndex}.  # TypeParam = {Type}
 *   {Test1Name}
 *   {Test2Name}
 */
bool TypeParameterizedTest::is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name)
{
    // Make sure there are at least 1 '/' delimiters, one between the
    // prefix and test case, and one between the test case and type
    // index. The prefix delim is optional
    size_t pos_delim = test_case_name.find(gtest_prefix_delim);
    if (pos_delim == std::string::npos)
    {
        return false;
    }
    // pos_delim = test_case_name.find(gtest_prefix_delim, pos_delim + 1);
    // if (pos_delim == std::string::npos)
    // {
    //     return false;
    // }

    // Now make sure the test case contains the signature
    return test_case_name.find(test_case_name_signature) != std::string::npos;
}

TypeParameterizedTest::TypeParameterizedTest(std::string const & fun_name)
// Test::Test{ fun_name }
{
    this->fun_name = fun_name;
    parse_fun_name();
}

TypeParameterizedTest::TypeParameterizedTest(std::string const & test_case_name, std::string const & test_name)
{
    // We need to split the test case into the Prefix, TestCase, and Type
    // First make sure if there is a prefix or not
    size_t pos_prefix = test_case_name.find(gtest_prefix_delim);
    size_t pos_suffix = test_case_name.find(gtest_prefix_delim, pos_prefix + 1);
    // Prefix exists
    if (pos_suffix != std::string::npos)
    {
        size_t pos_prefix = test_case_name.find(gtest_prefix_delim);
        prefix = test_case_name.substr(0, pos_prefix);
        prefix = strip_whitespace(prefix);
    }
    // Prefix doesn't exist, so suffix is the first '/'
    else
    {
        pos_suffix = pos_prefix;
        pos_prefix = -1;
        prefix = "";
    }

    // Now find the test case name, which is between the prefix and type
    this->test_case_name = test_case_name.substr(pos_prefix + 1, pos_suffix - pos_prefix - 1);
    this->test_case_name = strip_whitespace(this->test_case_name);

    // Now find the type index, which is right after the test case
    size_t pos_type = test_case_name.find(test_case_name_signature, pos_suffix + 1);
    std::string param_index_str = test_case_name.substr(pos_suffix + 1, pos_type - pos_suffix - 1);
    param_index_str = strip_whitespace(param_index_str);
    // Also erase the test case separator at the end ('.')
    param_index_str.erase(param_index_str.size() - 1);
    type_param_index = std::stoi(param_index_str);

    // Now find the type
    type_param = test_case_name.substr(pos_type + test_case_name_signature.size());
    type_param = strip_whitespace(type_param);

    // Check if the type needs to be truncated
    if (type_param.size() > max_param_length)
    {
        type_param = type_param.substr(0, max_param_length);
    }

    // this->test_case_name = test_case_name.substr(pos_prefix + 1, pos_type - pos_prefix - 1);
    // this->test_case_name = strip_whitespace(this->test_case_name);
    // Remove the trailing '.'
    // this->test_case_name.erase(this->test_case_name.size() - 1);
    this->test_name = strip_whitespace(test_name);

    // std::cout << prefix << '_' << this->test_case_name << '_' << this->test_name << '_' << type_param_index << '_' << type_param << std::endl;
}

std::string TypeParameterizedTest::get_gtest_filter_name()
{
    // parse_fun_name();
    if (prefix.empty())
    {
        return test_case_name + '/' + std::to_string(type_param_index) + '.' + test_name;
    }
    return prefix + '/' + test_case_name + '/' + std::to_string(type_param_index) + '.' + test_name;
    // return "*" + test_case_name + "*" + "." + test_name;
}

/**
 * TODO: I think that I have seen this method at several places.
 *
 * Returns the map key string that corresponds to this test. The map
 * key is suitable to be used in a hash map to be a unique identifier
 * for this test. Make sure that the map key can be constructed from
 * both the test executable (gtest_list_tests) as well as the bitcode
 * representation.
 * 
 * For type-parameterized tests, the map key will be {TestCase}_{TestName}_{TypeParam}
 */
std::string TypeParameterizedTest::get_map_key()
{
    return test_case_name + '_' + test_name + '_' + type_param;
}

void TypeParameterizedTest::parse_fun_name()
{
    // Usually the position of the signature is 0, but when the tests
    // are in a namespace, we need to find the position again.
    size_t pos_signature_begin = fun_name.find(signature_begin);
    // First keep track of where the TestCase begins, which is right after the begin signature
    size_t pos_test_case = signature_begin.size() + pos_signature_begin;

    // Find the location of the character right after the "_::", which
    // is the TestCase_::TestName separator
    size_t pos_test_name = fun_name.find("_::", pos_test_case) + 3;

    // Find the location of the character right after the angled
    // bracket, which is the TestName<Test separator
    size_t pos_type_start = fun_name.find('<', pos_test_name);

    // Find the last '>', which is the end of the type param
    size_t pos_type_end = fun_name.rfind('>');

    test_case_name = fun_name.substr(pos_test_case, pos_test_name - pos_test_case - 3);

    test_name = fun_name.substr(pos_test_name, pos_type_start - pos_test_name);
    type_param = fun_name.substr(pos_type_start + 1, pos_type_end - pos_type_start - 1);
    type_param = strip_whitespace(type_param);
    if (type_param.size() > max_param_length)
    {
        type_param = type_param.substr(0, max_param_length);
    }
}

}

}
