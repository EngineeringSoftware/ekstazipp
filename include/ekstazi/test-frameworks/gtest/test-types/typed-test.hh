#pragma once

#include "ekstazi/test-frameworks/gtest/test-types/test.hh"

#include <string>

namespace ekstazi::gtest
{

/**
 * We can tell what is a typed test by analyzing the signature for a normal gtest.
 * The typed test will be different in that it contains a type:
 * 
 * {namespace}::{TestCase}_{TestName}<{TestType}>::TestBody().
 */
class TypedTest : public Test
{
public:
    // We look for this to see if a test case is this test type
    static std::string const test_case_name_signature;

    static std::string const signature_begin;
    static std::string const signature_end;

    static uint32_t const MAX_TYPE_LENGTH;
    /**
     * Returns whether the given string is a typed test.
     */
    static bool is_test_from_bc(std::string const & name);

    /**
     * Returns whether the given test case indicates that the test case is a typed test.
     * Typed tests look like the following in --gtest_list_tests:
     * 
     * {TestCase}/{TypeParamIndex}.  # TypeParam = {Type}
     *   {Test1Name}
     *   {Test2Name}
     */
    static bool is_test_from_exec(std::string const & test_case_name, std::string const & first_test_name);

    using Test::Test;
    TypedTest(std::string const & name);
    TypedTest(std::string const & test_case_name, std::string const & test_name);

    /**
     * Returns the gtest filter representing the given test name.
     * For the typed test, we insert a '*' after the TestCase so the filter
     * looks like {TestCase}*.{TestName}. This handles all type suffixes.
     */
    virtual std::string get_gtest_filter_name() override;

    /**
     * Returns the map key string that corresponds to this test. The map key is suitable to be used in a hash map
     * to be a unique identifier for this test. Make sure that the map key can be constructed from both the
     * test executable (gtest_list_tests) as well as the bitcode representation.
     * 
     * For typed tests, the map key will be {TestCase}_{TestName}_{TypeParam}
     */
    virtual std::string get_map_key() override;

protected:
    /**
     * Parses the fun_name and sets the test_case_name and test_name fields.
     */
    virtual void parse_fun_name() override;
    
    std::string type_param;
    int type_param_index;

    // If the type was truncated by gtest or not
    bool type_is_truncated = false;
};


}

