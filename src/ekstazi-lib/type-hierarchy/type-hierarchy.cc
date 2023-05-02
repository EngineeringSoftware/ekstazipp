
#include "ekstazi/type-hierarchy/type-hierarchy.hh"

#include "ekstazi/utils/graph.hh"

#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>

namespace ekstazi
{

/**
 * Name of the derived hierarchy delimiter. This is used when printing
 * the type hierarchy and loading/saving from files.
 */
std::string const TypeHierarchy::derived_hierarchy_name{ "Derived Hierarchy:" };

/**
 * Name of the super hierarchy delimiter. This is used when printing
 * the type hierarchy and loading/saving from files.
 */
std::string const TypeHierarchy::super_hierarchy_name{ "Super Hierarchy:" };

/**
 * Delimiter used when saving and loading type hierarchies from files.
 */
char const TypeHierarchy::delim = ';';

TypeHierarchy::TypeHierarchy() :
m_super_adj_list{},
m_derived_adj_list{}
{

}

/**
 * Add an inheritance relationship for two types. The base type is the
 * base of the derived type.
 */
void TypeHierarchy::add_inheritance_relationship(std::string const & base_type_name, std::string const & derived_type_name)
{
    // base -(IS_BASE_OF)-> derived
    std::list<std::string> & derived_types_for_base = m_derived_adj_list[base_type_name];
    // std::list<std::string>::iterator it = std::find(derived_types_for_base.begin(), derived_types_for_base.end(), derived_type_name);
    // if (it == derived_types_for_base.end())
    // {
    //     derived_types_for_base.push_back(derived_type_name);
    // }
    derived_types_for_base.push_back(derived_type_name);

    // derived -(INHERITS_FROM)-> base
    std::list<std::string> & super_types_for_derived = m_super_adj_list[derived_type_name];
    // it = std::find(super_types_for_derived.begin(), super_types_for_derived.end(), base_type_name);
    // if (it == super_types_for_derived.end())
    // {
    //     super_types_for_derived.push_back(base_type_name);
    // }
    super_types_for_derived.push_back(base_type_name);
}

std::unordered_set<std::string> TypeHierarchy::get_derived_types(std::string const & base_type_name)
{
    return bfs(base_type_name, m_derived_adj_list);
}

std::unordered_set<std::string> TypeHierarchy::get_super_types(std::string const & derived_type_name)
{
    return bfs(derived_type_name, m_super_adj_list);
}

std::unordered_set<std::string> TypeHierarchy::get_all_related_types(std::string const & type_name)
{
    std::unordered_set<std::string> derived = get_derived_types(type_name);
    std::unordered_set<std::string> super = get_super_types(type_name);
    derived.insert(super.begin(), super.end());
    return derived;
}

/**
 * Return the max depth of the type hierarchy.
 */
uint32_t TypeHierarchy::max_depth()
{
    return ekstazi::max_distance(m_derived_adj_list);
}

/**
 * Return the average depth of the type hierarchy.
 */
double TypeHierarchy::average_depth()
{
    return average_distance(m_derived_adj_list);
}

/**
 * Return the total number of types/classes.
 */
uint32_t TypeHierarchy::size() const
{
    return num_nodes(m_derived_adj_list);
}

/**
 * Return the number of derived types/classes.
 */
uint32_t TypeHierarchy::num_derived_types() const
{
    return num_nonroot_nodes(m_derived_adj_list);
}

/**
 * Removes duplicates from adjacency list.
 */
void TypeHierarchy::remove_duplicates()
{
    remove_duplicates(m_derived_adj_list);
    remove_duplicates(m_super_adj_list);
}

/**
 * Prints the type hierarchy.
 */
void TypeHierarchy::print(std::ostream & os)
{
    os << derived_hierarchy_name << std::endl;
    for (std::pair<std::string, std::list<std::string>> const & p : m_derived_adj_list)
    {
        std::string const & base_type = p.first;
        os << base_type << delim;
        for (std::string const & derived_type : p.second)
        {
            os << derived_type << delim;
        }
        os << std::endl;
    }

    os << super_hierarchy_name << std::endl;

    for (std::pair<std::string, std::list<std::string>> const & p : m_super_adj_list)
    {
        std::string const & derived_type = p.first;
        os << derived_type << delim;
        for (std::string const & base_type : p.second)
        {
            os << base_type << delim;
        }
        os << std::endl;
    }
}

/**
 * Saves the type hierarchy to a file.
 */
void TypeHierarchy::save_file(std::string const & fname)
{
    std::ofstream ofs{ fname };
    print(ofs);
    ofs.close();
}

/**
 * Load class hierarchy from file.
 */
void TypeHierarchy::load_file(std::string const & fname)
{
    std::ifstream ifs{ fname };
    std::string line;
    while(std::getline(ifs, line))
    {
        if (line == derived_hierarchy_name)
        {
            while (std::getline(ifs, line))
            {
                if (line == super_hierarchy_name)
                {
                    break;
                }
                std::istringstream iss{ line };

                // Parse the base class
                std::string base_type;
                std::getline(iss, base_type, delim);

                // Parse all derived classes
                std::string derived_type;
                while (std::getline(iss, derived_type, delim))
                {
                    m_derived_adj_list[base_type].push_back(derived_type);
                }
            }
        }

        if (line == super_hierarchy_name)
        {
            while (std::getline(ifs, line))
            {
                std::istringstream iss{ line };

                // Parse the derived class
                std::string derived_type;
                std::getline(iss, derived_type, delim);

                // Parse all base classes
                std::string base_type;
                while (std::getline(iss, base_type, delim))
                {
                    m_super_adj_list[derived_type].push_back(base_type);
                }
            }
        }
    }
}

 /**
 * Removes duplicates from adjacency list.
 */
void TypeHierarchy::remove_duplicates(std::unordered_map<std::string, std::list<std::string>> & adj_list)
{
    for (std::unordered_map<std::string, std::list<std::string>>::iterator it = adj_list.begin(); it != adj_list.end(); ++it)
    {
        it->second.sort();
        it->second.unique();
    }
}

std::unordered_set<std::string> TypeHierarchy::get_all_children(std::string const & start_node, std::unordered_map<std::string, std::list<std::string>> const & adj_list)
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
        auto search = adj_list.find(cur_node);
        // If not found, continue
        if (search == adj_list.end())
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

bool TypeHierarchy::contains(std::string const & class_name)
{
    auto it = m_derived_adj_list.find(class_name);
    if (it != m_derived_adj_list.end())
    {
        return true;
    }

    // Now search through all of the nodes
    for (auto & p : m_derived_adj_list)
    {
        for (std::string const & name : p.second)
        {
            if (class_name == name)
            {
                return true;
            }
        }
    }

    return false;
}

}
