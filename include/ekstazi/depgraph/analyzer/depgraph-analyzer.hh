#pragma once

#include "ekstazi/depgraph/depgraph.hh"
#include "ekstazi/depgraph/function.hh"

#include <string>
#include <map>

namespace ekstazi
{

class DepgraphAnalyzer
{
public:
    DepgraphAnalyzer(std::string const & module_name);
    /**
     * Load the dependency graph and functions for a module.
     */
    void load();

    /**
     * Get all of the functions that depend on a function.
     */
    std::unordered_set<std::string> get_dependents(std::string const & fun_name);

    /**
     * Get all of the functions that a function depends on.
     */
    std::unordered_set<std::string> get_dependencies(std::string const & fun_name);
    
protected:
    std::string module_name;

    ekstazi::DependencyGraph new_depgraph;
    ekstazi::DependencyGraph old_depgraph;

    std::map<std::string, ekstazi::Function> new_functions;
    std::map<std::string, ekstazi::Function> old_functions;
};

}
