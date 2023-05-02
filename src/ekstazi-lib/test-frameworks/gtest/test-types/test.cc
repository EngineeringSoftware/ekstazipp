
#include "ekstazi/test-frameworks/gtest/test-types/test.hh"

#include "ekstazi/test-frameworks/gtest/test-types/value-parameterized-test.hh"
#include "ekstazi/test-frameworks/gtest/test-types/type-parameterized-test.hh"
#include "ekstazi/test-frameworks/gtest/test-types/typed-test.hh"

#include <iostream>

namespace ekstazi
{

namespace gtest
{
/**
 * Maximum length of any parameter (type or value)
 */
uint32_t const Test::max_param_length = 250;

/**
 * The Test class represents any gtest that is instantiated.
 * Tests look like the following:
 * 
 * {namespace}::{TestCase}_{TestName}_Test::TestBody()
 */
std::string const Test::signature = "_Test::TestBody()";

// Delimiter between the test prefix and test case.
char const Test::gtest_prefix_delim = '/';

bool Test::is_test_from_bc(std::string const & fun_name, std::string const & signature)
{
    // We know that TestBody() is at the end of the string
    size_t pos = fun_name.find(signature, fun_name.size() - signature.size());
    if (pos != std::string::npos)
    {
        return true;
    }

    return false;
}

/**
 * Returns whether the given test case indicates that the test case is
 * a normal test.  Tests look like the following in
 * --gtest_list_tests:
 * 
 * {TestCase}.
 *   {Test1Name}
 *   {Test2Name}
 */
bool Test::is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name)
{
    return !ValueParameterizedTest::is_test_from_exec(test_case_name, first_test_name)
        && !TypeParameterizedTest::is_test_from_exec(test_case_name, first_test_name)
        && !TypedTest::is_test_from_exec(test_case_name, first_test_name);
}

Test::Test()
{

}

Test::Test(std::string const & fun_name) : 
fun_name { fun_name }
{
    this->fun_name = fun_name;
    parse_fun_name();
}

Test::Test(std::string const & test_case_name, std::string const & test_name)
{
    this->test_case_name = strip_whitespace(test_case_name);
    // Remove the trailing '.' after the test case
    this->test_case_name.erase(this->test_case_name.size() - 1);
    this->test_name = strip_whitespace(test_name);
}

Test::~Test()
{
}

std::string Test::get_testbody_fun_name()
{
    return fun_name;
}

std::string Test::get_gtest_filter_name()
{
    // parse_fun_name();
    return test_case_name + "." + test_name;
}

std::string Test::get_test_case_name() const
{
    return test_case_name;
}

std::string Test::get_test_name() const
{
    return test_name;
}

/**
 * Returns the map key string that corresponds to this test. The map
 * key is suitable to be used in a hash map to be a unique identifier
 * for this test. Make sure that the map key can be constructed from
 * both the test executable (gtest_list_tests) as well as the bitcode
 * representation.
 */
std::string Test::get_map_key()
{
    return test_case_name + '_' + test_name;
}

/**
 * Parses the fun_name and sets the test_case_name and test_name
 * fields.  Note that for a normal test, we cannot differentiate
 * between the test case and test name if they contain an
 * underscore. Thus, we just assume the first underscore splits the
 * test case and test name, while the last underscore splits the test
 * name and the "test" keyword. This way, when we concatenate the test
 * case and test name, it will exist in our tests map.
 */
void Test::parse_fun_name()
{
    // Remove the signature and namespace from the function
    size_t pos_signature = fun_name.rfind(signature);
    fun_name = fun_name.substr(0, pos_signature);
    fun_name = strip_namespace(fun_name);

    // Find the location of the first underscore, which is the
    // TestCase_TestName separator
    size_t pos_test_case_delim = fun_name.find('_');

    // Find the location of the character right after the last
    // underscore, which is the TestName_Test separator
    // size_t pos_end = fun_name.rfind('_');

    test_case_name = fun_name.substr(0, pos_test_case_delim);
    // Remember to strip the namespace from the beginning of the test case
    // test_case_name = strip_namespace(test_case_name);
    // std::cout << "Test Case: " << test_case_name << std::endl;

    test_name = fun_name.substr(pos_test_case_delim + 1);
    // std::cout << "Test Name: " << test_name << std::endl;
}

/**
 * Strips the preceding namespaces from a string.
 * e.g. ekstazi::TestCase -> TestCase
 * 
 * This function also saves the stripped namespace to the
 * namespace_name member.
 */
std::string Test::strip_namespace(std::string const & str)
{
    size_t pos_last_namespace = str.rfind("::");
    // size_t pos_last_colon = str.find_last_of(':');
    if (pos_last_namespace == std::string::npos)
    {
        return str;
    }
    namespace_name = str.substr(0, pos_last_namespace);
    return str.substr(pos_last_namespace + 2);
}

/**
 * Strips the leading and trailing whitespace from a string.
 */
std::string Test::strip_whitespace(std::string const & str)
{
    // Find the first non-whitespace
    size_t pos_first = str.find_first_not_of(" \t");

    // Fand the last non-whitespace
    size_t pos_last = str.find_last_not_of(" \t");
    // We ned to add 1 to include the last character
    return str.substr(pos_first, pos_last - pos_first + 1);
}

}

}
