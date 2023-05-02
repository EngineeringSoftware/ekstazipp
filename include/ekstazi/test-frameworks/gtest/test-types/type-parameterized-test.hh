#pragma once

#include "ekstazi/test-frameworks/gtest/test-types/test.hh"

#include <string>

namespace ekstazi
{

namespace gtest
{

/**
 * Type-Parameterized tests look like this in bitcode:
 * 
 * {namespace}::gtest_case_::{TestCase}_::{TestName}<char>::TestBody()
 * 
 * Thus, we just need to look for the beginning and end signatures.
 */

class TypeParameterizedTest : public Test
{
public:
    // We look for this to see if a test case is this type
    static std::string const test_case_name_signature;

    // We look for this to see if a test name is this test type
    static std::string const test_name_signature;

    static std::string const signature_begin;
    static std::string const signature_end;

    static bool is_test_from_bc(std::string const & fun_name);

    /**
     * Returns whether the given test case indicates that the test case is a type-parameterized test.
     * Type-parameterized tests look like the following in --gtest_list_tests:
     * 
     * {Prefix}/{TestCase}/{TypeParamIndex}.  # TypeParam = {Type}
     *   {Test1Name}
     *   {Test2Name}
     */
    static bool is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name);

    using Test::Test;
    TypeParameterizedTest(std::string const & name);
    TypeParameterizedTest(std::string const & test_case_name, std::string const & test_name);

    /**
     * Returns the gtest filter representing the given test name.
     * For the Value-Parameterized Test, we insert a "*" before the
     * TestCase, so the resulting filter looks like *{TestCase}.{TestName}.
     * This handles all user-assigned prefixes.
     */
    virtual std::string get_gtest_filter_name() override;

    /**
     * Returns the map key string that corresponds to this test. The map key is suitable to be used in a hash map
     * to be a unique identifier for this test. Make sure that the map key can be constructed from both the
     * test executable (gtest_list_tests) as well as the bitcode representation.
     * 
     * For type-parameterized tests, the map key will be {TestCase}_{TestName}_{TypeParam}
     */
    virtual std::string get_map_key() override;

protected:
    /**
     * Parses the fun_name and sets the test_case_name and test_name fields.
     */
    virtual void parse_fun_name() override;
    
    // Type-parameterized test cases have a prefix
    std::string prefix;

    // Type-parameterized tests also have a type
    std::string type_param;

    // Type-parameterized test names have a type index
    uint32_t type_param_index;

    // If the type was truncated by gtest or not
    bool type_is_truncated = false;
};

}


}

