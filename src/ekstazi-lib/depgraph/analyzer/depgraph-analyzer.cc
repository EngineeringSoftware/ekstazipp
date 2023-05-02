#include <iostream>
#include <string>
#include <set>

#include "ekstazi/depgraph/analyzer/depgraph-analyzer.hh"
#include "ekstazi/constants.hh"

namespace ekstazi
{

DepgraphAnalyzer::DepgraphAnalyzer(std::string const & module_name)
: module_name{ module_name }
{
    this->module_name = module_name + ".0.5.precodegen.bc";
}

void DepgraphAnalyzer::load()
{
    std::string new_depgraph_fname = ekstazi::EKSTAZI_DIRNAME + "/" + module_name + "." + ekstazi::DEPGRAPH_FNAME;
    new_depgraph.load_file(new_depgraph_fname);

    std::string old_depgraph_fname = new_depgraph_fname + '.' + ekstazi::OLD_SUFFIX;
    old_depgraph.load_file(old_depgraph_fname);

    std::string new_functions_fname = ekstazi::EKSTAZI_DIRNAME + '/' + module_name + '.' + ekstazi::FUNCTIONS_FNAME;
    new_functions = ekstazi::Function::load_file(new_functions_fname);

    std::string old_functions_fname = new_functions_fname + '.' + ekstazi::OLD_SUFFIX;
    old_functions = ekstazi::Function::load_file(old_functions_fname);
}

/**
 * Get all of the functions that depend on a function.
 */
std::unordered_set<std::string> DepgraphAnalyzer::get_dependents(std::string const & fun_name)
{
    if (old_depgraph.empty())
    {
        std::cout << "Warning: no old dependency graph detected. Returning new dependents" << std::endl;
        return new_depgraph.get_all_dependents(fun_name);
    }
    return old_depgraph.get_all_dependents(fun_name);
}

/**
 * Get all of the functions that a function depends on.
 */
std::unordered_set<std::string> DepgraphAnalyzer::get_dependencies(std::string const & fun_name)
{
    if (old_depgraph.empty())
    {
        std::cout << "Warning: no old dependency graph detected. Returning new dependencies" << std::endl;
        ekstazi::DependencyGraph reversed = new_depgraph.reverse();
        return reversed.get_all_dependents(fun_name);
    }
    ekstazi::DependencyGraph reversed = old_depgraph.reverse();
    return reversed.get_all_dependents(fun_name);
}


}   // namespace ekstazi

int main(int argc, char* argv[])
{
    std::string module_name = argv[1];
    std::string function_name = argv[2];

    std::cout << "Module Name: " << module_name << std::endl;
    std::cout << "Function Name: " << function_name << std::endl;

    ekstazi::DepgraphAnalyzer depgraph_analyzer{ module_name };
    depgraph_analyzer.load();

    std::unordered_set<std::string> dependents = depgraph_analyzer.get_dependents(function_name);
    std::cout << "Dependents:" << std::endl;
    for (std::string const & dependent : dependents)
    {
        std::cout << dependent << std::endl;
    }

    std::unordered_set<std::string> dependencies = depgraph_analyzer.get_dependencies(function_name);
    std::cout << "Dependencies:" << std::endl;
    for (std::string const & dependency : dependencies)
    {
        std::cout << dependency << std::endl;
    }

    return 0;
}
