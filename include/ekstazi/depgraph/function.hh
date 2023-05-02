#pragma once

#include <string>
#include <map>
#include <unordered_set>

namespace ekstazi
{

class Function
{
public:
    static std::map<std::string, Function> load_file(std::string const & fname);
    static void save_file(std::map<std::string, Function> const & functions, std::string const & fname);

    /**
     * Returns whether or not a given function is a constructor.
     * Functions are constructors if they have the same class name
     * and function name, e.g. A::A().
     */
    static bool is_constructor(std::string const & fun_name);

    /**
     * Returns the containing class and function name for a member function.
     * 
     * @param {fun_name} The demangled function name.
     * @param {demangle} Whether or not we should demangle the function name.
     */
    static std::pair<std::string, std::string> split_class_name(std::string const & fun_name, bool should_demangle = true);

    /**
     * Returns a set of functions that have been modified given a set of old and new functions. Modified means that either 
     * the functions don't exist from old to new and vice versa (similar to set XOR), or the computed hash changed.
     */
    static std::unordered_set<std::string> get_modified_functions(std::map<std::string, Function> const & old_funs, std::map<std::string, Function> const & new_funs);

    Function(std::string const & name, std::string const & fname, std::string const & checksum);

    std::string const & name() const;

    std::string const & checksum() const;

    std::string const & filename() const;

    bool operator==(Function const & fun) const;

protected:
    std::string m_name;

    std::string m_filename;
    std::string m_checksum;
};


}

namespace std
{
    // hash implementation
    template <>
    struct hash<ekstazi::Function>
    {
        size_t operator()(ekstazi::Function const & fun) const;
    };
}
