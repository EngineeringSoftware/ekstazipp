/**
 * This script takes the ekstazi++ results for a project by parsing the metadata
 * and computing:
 * - The number of tests
 * - The number of test cases
 * 
 * There are two modes of modularity:
 * - function
 * - class
 * 
 */

#include "ekstazi/constants.hh"
#include "ekstazi/depgraph/depgraph.hh"
#include "ekstazi/depgraph/function.hh"
#include "ekstazi/type-hierarchy/type-hierarchy.hh"
#include "ekstazi/test-frameworks/gtest/gtest-adapter.hh"
#include "ekstazi/test-frameworks/gtest/test-types/test.hh"

#include <string>
#include <iostream>
#include <fstream>
#include <map>

using namespace ekstazi;

class ResultsAnalyzer
{
public:
    ResultsAnalyzer(std::string const & ekstazi_dir, std::string const & bc_fname) :
    m_ekstazi_dir{ ekstazi_dir },
    m_bc_fname{ bc_fname }
    {
        // The module name is just the stripped bitcode file name
        size_t last_dir_index = bc_fname.find_last_of('/');
        if (last_dir_index != std::string::npos)
        {
            m_module_name = bc_fname.substr(last_dir_index + 1, bc_fname.length());
        }

        // std::cout << m_module_name << std::endl;
    }

    void analyze_results()
    {
        // Load metadata first
        m_old_degraph.load_file(m_ekstazi_dir + '/' + m_module_name + '.' + DEPGRAPH_FNAME + '.' + OLD_SUFFIX);
        m_new_degraph.load_file(m_ekstazi_dir + '/' + m_module_name + '.' + DEPGRAPH_FNAME);
        
        m_old_functions = Function::load_file(m_ekstazi_dir + '/' + m_module_name + '.' + FUNCTIONS_FNAME + '.' + OLD_SUFFIX);
        m_new_functions = Function::load_file(m_ekstazi_dir + '/' + m_module_name + '.' + FUNCTIONS_FNAME);

        std::string modified_functions_fname = m_ekstazi_dir + '/' + m_module_name + '.' + MODIFIED_FUNS_FNAME;
        std::ifstream ifs{ modified_functions_fname };
        std::string line;
        while (std::getline(ifs, line))
        {
            m_modified_functions.insert(line);
        }
        ifs.close();
        
        m_old_type_hierarchy.load_file(m_ekstazi_dir + '/' + m_module_name + '.' + TYPE_HIERARCHY_FNAME + '.' + OLD_SUFFIX);
        m_new_type_hierarchy.load_file(m_ekstazi_dir + '/' + m_module_name + '.' + TYPE_HIERARCHY_FNAME);

        // Register all of the gtest tests
        m_gtest_adapter.register_tests(m_bc_fname);
        // gtest_adapter.register_tests(new_functions);

        // Now we need to filter out the test functions.
        std::unordered_map<std::string, std::shared_ptr<ekstazi::gtest::Test>> modified_tests = m_gtest_adapter.get_modified_tests(m_modified_functions);
        std::cout << "num_tests_fun_test: " << modified_tests.size() << std::endl;

        modified_tests = m_gtest_adapter.get_modified_tests_sel_case(m_modified_functions);
        std::cout << "num_tests_fun_case: " << modified_tests.size() << std::endl;

        // Now use class-level modularity. We add all functions to m_modified_functions
        std::unordered_set<std::string> modified_functions_class;
        for (std::string const & fun_name : m_modified_functions)
        {
            modified_functions_class.insert(fun_name);
            // First get the class name of the function
            size_t pos_name_end = fun_name.find('(');
            std::string fun_name_no_params = fun_name.substr(0, pos_name_end);
            size_t pos_class = fun_name_no_params.rfind("::");
            // If doesn't exist, then can't be a class
            if (pos_class == std::string::npos)
            {
                continue;
            }
            // Now the name could be a namespace or a class
            std::string class_name = fun_name_no_params.substr(0, pos_class);
            if (m_old_type_hierarchy.contains(class_name) || m_new_type_hierarchy.contains(class_name))
            {
                // If class name is a class, then add all functions belonging to class to the modified functions set.
                for (auto & p : m_old_functions)
                {
                    if (p.first.find(class_name) != std::string::npos)
                    {
                        modified_functions_class.insert(p.first);
                    }
                }
                for (auto & p : m_new_functions)
                {
                    if (p.first.find(class_name) != std::string::npos)
                    {
                        modified_functions_class.insert(p.first);
                    }
                }
            }
            
        }

        // std::cerr << m_modified_functions.size() << ", " << modified_functions_class.size() << std::endl;

        modified_tests = m_gtest_adapter.get_modified_tests(modified_functions_class);
        std::cout << "num_tests_class_test: " << modified_tests.size() << std::endl;

        modified_tests = m_gtest_adapter.get_modified_tests_sel_case(modified_functions_class);
        std::cout << "num_tests_class_case: " << modified_tests.size() << std::endl;

        // Generate the number of 
    }

    void analyze_num_tests()
    {

    }



protected:
    std::string m_ekstazi_dir;
    std::string m_bc_fname;
    std::string m_module_name;

    DependencyGraph m_old_degraph;
    DependencyGraph m_new_degraph;

    std::map<std::string, Function> m_old_functions;
    std::map<std::string, Function> m_new_functions;
    std::unordered_set<std::string> m_modified_functions;

    TypeHierarchy m_old_type_hierarchy;
    TypeHierarchy m_new_type_hierarchy;

    ekstazi::gtest::GtestAdapter m_gtest_adapter;

};

/**
 * This script takes in two arguments:
 * - The ekstazi metadata directory.
 * - The full path to the test bitcode file.
 * 
 * For example:
 * ./results-analyzer /home/user/project-deps/build/.ekstazi /home/user/project/build/test1.bc
 * 
 */
int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Must pass in two arguments, the path to the metadata directory and the path to the bitcode." << std::endl;
        exit(1);
    }

    ResultsAnalyzer ra{ argv[1], argv[2] };
    ra.analyze_results();

}
