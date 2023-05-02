
#include "ekstazi/depgraph/depgraph.hh"

#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>

namespace ekstazi
{

DependencyGraph::DependencyGraph() :
m_adj_list{}
{

}

DependencyGraph::DependencyGraph(DependencyGraph const & other) :
m_adj_list{ other.m_adj_list }
{

}

/**
 * Adds a dependency relationship to the current graph. The
 * relationship is that the src function is depended on by the dst
 * function, or that the dst function depends on the src function.
 */
void DependencyGraph::add_dependency(std::string const & function_src, std::string const & function_dst)
{
    // Don't insert if src and dst are the same
    if (function_src == function_dst)
    {
        return;
    }
    // std::cout << "Adding dependency" << std::endl;
    std::list<std::string> & dependents = m_adj_list[function_src];

    // Insert into dependents if it doesn't already exist
    // std::list<std::string>::iterator it = std::find(dependents.begin(), dependents.end(), function_dst);
    // if (it == dependents.end())
    // {
    //     dependents.push_back(function_dst);
    // }

    dependents.push_back(function_dst);
    // auto search = m_adj_list.find(function_src);
    // if (search == m_adj_list.end())
    // {   // not found, so create a new node and list
    //     // std::cout << "Creating node for " << function_src << std::endl;
    //     m_adj_list.insert({ function_src, std::list<std::string>{} });
    //     // "search" for the new node
    //     search = m_adj_list.find(function_src);
    // }

    // auto & connected_functions = search->second;
    // connected_functions.push_back(function_dst);
}

std::unordered_set<std::string> DependencyGraph::get_all_dependents(std::string const & start_node)
{
    std::unordered_set<std::string> dependents{};

    // Conduct a breadth-first search
    std::queue<std::string> visit_queue{};
    std::unordered_set<std::string> visited{};

    visit_queue.push(start_node);

    while (!visit_queue.empty())
    {
        // Get front element and mark as visited
        std::string cur_node = visit_queue.front();
        // std::cout << "Searching for children of " << cur_node << std::endl;
        visit_queue.pop();
        visited.insert(cur_node);

        // Get all direct dependents
        auto search = m_adj_list.find(cur_node);
        // If not found, continue
        if (search == m_adj_list.end())
        {
            // std::cout << "Not found in adjacency list: " << cur_node << std::endl;
            continue;
        }

        std::list<std::string> const & direct_dependents = search->second;
        dependents.insert(direct_dependents.begin(), direct_dependents.end());

        // Add to visit_queue queue if not already visited
        // std::cout << "Direct dependents of " << cur_node << ": " << std::endl;
        for (std::string const & dependent : direct_dependents)
        {
            // std::cout << dependent << std::endl;
            auto search = visited.find(dependent);
            if (search == visited.end())
            {
                visit_queue.push(dependent);
                // std::cout << "Added " << dependent << " to the visit queue." << std::endl;
            }
        }
    }

    return dependents;
}

bool DependencyGraph::empty() {
    return m_adj_list.empty();
}

/**
 * Reverses a dependency graph and returns it (original remains
 * unchanged).
 */
DependencyGraph DependencyGraph::reverse()
{
    DependencyGraph reversed;
    // To reverse the dependency graph, we iterate through the
    // adjacency list and make new connections backwards from the list
    // node to the key.
    for (auto pair: m_adj_list)
    {
        for (std::string const & function : pair.second)
        {
            // Connect from function -> original key
            reversed.add_dependency(function, pair.first);
        }
    }
    return reversed;
}

/**
 * Removes duplicates from the dependency graph.
 */
void DependencyGraph::remove_duplicates()
{
    for (std::unordered_map<std::string, std::list<std::string>>::iterator it = m_adj_list.begin(); it != m_adj_list.end(); ++it)
    {
        it->second.sort();
        it->second.unique();
    }
}

/**
 * Returns whether or not a dependency relation exists.
 */
bool DependencyGraph::exists_dependency(std::string const & function_src, std::string const & function_dst)
{
    std::unordered_map<std::string, std::list<std::string>>::iterator it = m_adj_list.find(function_src);
    if (it == m_adj_list.end())
    {
        return false;
    }

    for (std::string const & fun : it->second)
    {
        if (fun == function_dst)
        {
            return true;
        }
    }

    return false;
}

void DependencyGraph::print()
{
    for (auto pair : m_adj_list)
    {
        std::cout << pair.first << std::endl;

        std::list<std::string> connected_functions = pair.second;
        for (auto fun : connected_functions)
        {
            std::cout << " -> " << fun << std::endl;
        }
    }
}

void DependencyGraph::load_file(std::string const & fname)
{
    std::ifstream ifs { fname };
    char delim = ';';
    char dep_delim = ';';

    std::string line;
    while (std::getline(ifs, line))
    {
        std::istringstream iss{ line };

        // First parse the source node
        std::string src_name;
        std::getline(iss, src_name, delim);

        // Now get all of the connected nodes
        std::string dst_name;
        while (std::getline(iss, dst_name, dep_delim))
        {
            add_dependency(src_name, dst_name);
        }
    }
}

/**
 * Saves the dependnency graph to a file.
 */
void DependencyGraph::save_file(std::string const & fname)
{
    std::ofstream ofs { fname };
    char delim = ';';
    char dep_delim = ';';

    for (auto const & pair : m_adj_list)
    {
        ofs << pair.first << delim;

        std::list<std::string> const & connected_functions = pair.second;

        int i = 0;
        for (auto const & fun : connected_functions)
        {
            ofs << fun;
            
            ++i;
            if (i < connected_functions.size())
            {
                ofs << delim;
            }
        }

        ofs << std::endl;
    }

    ofs.close();
}

}
