#pragma once

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>
#include <iostream>


namespace ekstazi
{

class TypeHierarchy
{
public:
    /**
     * Name of the derived hierarchy delimiter. This is used when printing the type hierarchy and
     * loading/saving from files.
     */
    static std::string const derived_hierarchy_name;

    /**
     * Name of the super hierarchy delimiter. This is used when printing the type hierarchy and
     * loading/saving from files.
     */
    static std::string const super_hierarchy_name;

    /**
     * Delimiter used when saving and loading type hierarchies from files.
     */
    static char const delim;

    TypeHierarchy();

    /**
     * Add an inheritance relationship for two types. The base type is the base of the derived type.
     */
    void add_inheritance_relationship(std::string const & base_type_name, std::string const & derived_type_name);

    std::unordered_set<std::string> get_derived_types(std::string const & base_type_name);

    std::unordered_set<std::string> get_super_types(std::string const & derived_type_name);

    std::unordered_set<std::string> get_all_related_types(std::string const & type_name);

    /**
     * Return the max depth of the type hierarchy.
     */
    uint32_t max_depth();

    /**
     * Return the average depth of the type hierarchy.
     */
    double average_depth();

    /**
     * Return the total number of types/classes.
     */
    uint32_t size() const;

    /**
     * Return the number of derived types/classes.
     */
    uint32_t num_derived_types() const;

    /**
     * Removes duplicates from adjacency list.
     */
    void remove_duplicates();

    /**
     * Prints the type hierarchy.
     */
    void print(std::ostream & os = std::cout);

    /**
     * Saves the type hierarchy to a file.
     */
    void save_file(std::string const & fname);

    /**
     * Load class hierarchy from file.
     */
    void load_file(std::string const & fname);

    bool contains(std::string const & class_name);

protected:
    /**
     * Traverses an adjacency list and returns all children.
     */
    std::unordered_set<std::string> get_all_children(std::string const & name, std::unordered_map<std::string, std::list<std::string>> const & adj_list);

    /**
     * Removes duplicates from adjacency list.
     */
    void remove_duplicates(std::unordered_map<std::string, std::list<std::string>> & adj_list);

    /**
     * We maintain two adjacency lists, one to represent supertype relationships
     * and one to represent derived relationships.
     */
    // Represents derived relationships where node1 -(IS_BASE_OF) -> node2
    std::unordered_map<std::string, std::list<std::string>> m_derived_adj_list;

    // Represents supertype relationships where node1 -(INHERITS_FROM)-> node2
    std::unordered_map<std::string, std::list<std::string>> m_super_adj_list;
    
};


}
