#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <list>

namespace ekstazi
{

/**
 * The Ekstazi dependency graph represents the dependencies between functions generated
 * from LLVM IR code. The graph is a directed graph, and the relationship between any two
 * nodes is defined by the fact that the source node is depended on by the destination node,
 * or in other words the destination node depends on the source node.
 * 
 * e.g. given the following relationship between two functions:
 * fun B():
 *      A()
 * 
 * The resulting dependency looks like:
 * A -> B
 * 
 * Function B calls function A, so A is depended on by B.
 */
class DependencyGraph
{
public:
    DependencyGraph();
    DependencyGraph(DependencyGraph const & other);

    /**
     * Finds all dependents for the given start node. Recursively searches through the graph
     * until there are no more nodes.
     */
    std::unordered_set<std::string> get_all_dependents(std::string const & start_node);

    /**
     * Adds a dependency relationship to the current graph. The relationship is that
     * the src function is depended on by the dst function, or that the dst function
     * depends on the src function.
     */
    void add_dependency(std::string const & function_src, std::string const & function_dst);

    /**
     * Returns whether or not the dependency graph is empty.
     */
    bool empty();

    /**
     * Reverses a dependency graph and returns it (original remains unchanged).
     */
    DependencyGraph reverse();

    /**
     * Removes duplicates from the dependency graph.
     */
    void remove_duplicates();

    /**
     * Returns whether or not a dependency relation exists.
     */
    bool exists_dependency(std::string const & function_src, std::string const & function_dst);

    /**
     * Loads the dependency graph from a file. The format of the file should be a list of
     * functions followed by their dependencies on each line, similar to the structure
     * of an adjacency list.
     */
    void load_file(std::string const & fname);

    /**
     * Saves the dependnency graph to a file.
     */
    void save_file(std::string const & fname);

    void print();
protected:

    std::unordered_map<std::string, std::list<std::string>> m_adj_list;

};

}
