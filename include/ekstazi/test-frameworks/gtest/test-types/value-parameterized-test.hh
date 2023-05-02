
#pragma once

#include "ekstazi/test-frameworks/gtest/test-types/test.hh"

#include <string>

namespace ekstazi
{

namespace gtest
{

/**
 * In bitcode, value-parameterized tests look just like normal gtest tests if we only look for ::TestBody().
 * However, we can differentiate value-parameterized tests by looking for the following signature:
 * 
 * testing::internal::ParameterizedTestFactory<{namespace}::{TestCase}_{TestName}_Test>::CreateTest()
 * 
 * Thus, we need to register that this is a value-parameterized test when first parsing through the bitcode.
 * 
 */

class ValueParameterizedTest : public Test
{
public:
    // We look for this to see if a test name is this test type
    static std::string const test_name_signature;

    static std::string const signature_begin;
    static std::string const signature_end;

    static bool is_test_from_bc(std::string const & fun_name);

    using Test::Test;
    ValueParameterizedTest(std::string const & name);
    ValueParameterizedTest(std::string const & test_case_name, std::string const & test_name);

    /**
     * Returns whether the given test case indicates that the test case is a typed test.
     * Typed tests look like the following in --gtest_list_tests:
     * 
     * {Prefix}/{TestCase}.
     *   {Test1Name}/0  # GetParam() = {Param1}
     *   {Test1Name}/1  # GetParam() = {Param2}
     */
    static bool is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name);

    /**
     * Returns the gtest filter representing the given test name.
     * For the Value-Parameterized Test, we insert a "*" before the
     * TestCase, so the resulting filter looks like *{TestCase}.{TestName}.
     * This handles all user-assigned prefixes.
     */
    virtual std::string get_gtest_filter_name() override;

    /**
     * Returns the name of the test body function name. Since most tests look for TestBody() as their signature,
     * this will return the function name in most of the cases. However, for the Value-Parameterized Test, we
     * will need to actually parse the TestCase and TestName and return the TestBody() equivalent.
     */
    virtual std::string get_testbody_fun_name() override;

protected:
    /**
     * Parses the fun_name and sets the test_case_name and test_name fields.
     */
    // virtual void parse_fun_name() override;

    std::string prefix;
    std::string param;
    std::string param_index;
};

}


}

