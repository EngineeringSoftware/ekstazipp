
#include "ekstazi/depgraph/function.hh"
#include "ekstazi/utils/mangle.hh"

#include <fstream>
#include <iostream>
#include <sstream>

namespace ekstazi
{

void Function::save_file(std::map<std::string, Function> const & functions, std::string const & fname)
{
    std::ofstream ofs{ fname };

    char delim = ';';

    for (std::pair<std::string, Function> const & p : functions)
    {
        Function f = p.second;
        ofs << f.name() << delim << f.filename() << delim << f.checksum() << std::endl;
    }

    ofs.close();
}

/**
 * Returns whether or not a given function is a constructor.
 * Functions are constructors if they have the same class name
 * and function name, e.g. A::A().
 * 
 * The mangled function name is a constructor if it contains:
 * C1, C2, or C3.
 * 
 * @param {fun_name} the mangled function name.
 */
bool Function::is_constructor(std::string const & fun_name)
{
    // Mangled name MUST contain 'C1', 'C2', or 'C3'
    if (fun_name.find("C1") == std::string::npos &&
        fun_name.find("C2") == std::string::npos &&
        fun_name.find("C3") == std::string::npos)
    {
        return false;
    }

    std::pair<std::string, std::string> p = split_class_name(fun_name);

    // Find '::' if it exists in the class name, and strip out namespaces
    size_t pos_namespace_end = p.first.rfind("::");
    if (pos_namespace_end == std::string::npos)
    {
        pos_namespace_end = 0;
    }
    else
    {
        // Set pos_namespace_end to the beginning of the class name
        pos_namespace_end += 2;
    }

    // Now check that class name == fun name
    std::string class_name_no_ns = p.first.substr(pos_namespace_end);

    return class_name_no_ns == p.second;
}

/**
 * Returns the containing class and function name for a member function.
 * 
 * @param {fun_name} The function name.
 * @param {demangle} Whether or not we should demangle the function name.
 */
std::pair<std::string, std::string> Function::split_class_name(std::string const & fun_name, bool should_demangle)
{
    std::pair<std::string, std::string> res;

    std::string demangled_name = fun_name;
    if (should_demangle)
    {
        demangled_name = demangle(fun_name);
    }
    // Find location of opening parenthesis '('
    size_t pos_first_arg = demangled_name.find('(');
    // Find location of last '::', which separates the class name from
    // the function name.
    size_t pos_fun_separator = demangled_name.rfind("::", pos_first_arg);

    if (pos_fun_separator == std::string::npos)
    {
        res.first = demangled_name.substr(0, pos_fun_separator);
        res.second = "";
        return res;
    }

    res.first = demangled_name.substr(0, pos_fun_separator);
    res.second = demangled_name.substr(pos_fun_separator + 2, pos_first_arg - pos_fun_separator - 2);

    return res;
}

std::map<std::string, Function> Function::load_file(std::string const & fname)
{
    std::ifstream ifs{ fname };
    std::map<std::string, Function> functions{};
    char delim = ';';
    
    std::string line;
    // int count = 0;
    while (std::getline(ifs, line))
    {
        std::istringstream iss{ line };

        // Split the string by the delimiter
        std::string name;
        std::getline(iss, name, delim);

        std::string fname;
        std::getline(iss, fname, delim);

        std::string checksum;
        std::getline(iss, checksum, delim);

        Function f{ name, fname, checksum };

        // std::cout << "Name: " << f.name() << std::endl;
        // std::cout << "FName: " << f.filename() << std::endl;
        // std::cout << "Checksum: " << f.checksum() << std::endl;
        // ++count;
        // std::cout << "Count: " << count << std::endl;
        
        functions.insert({ f.name(), f });
    }

    ifs.close();

    return functions;
}

/**
 * Returns a set of functions that have been modified given a set of
 * old and new functions. Modified means that either the functions
 * don't exist from old to new and vice versa (similar to set XOR), or
 * the computed hash changed.
 */
std::unordered_set<std::string> Function::get_modified_functions(std::map<std::string, Function> const & old_functions, std::map<std::string, Function> const & new_functions)
{   
    std::unordered_set<std::string> modified_functions;

    // Find changed functions
    for (std::pair<std::string, ekstazi::Function> const & p : old_functions)
    {
        std::string const & old_fun_name = p.first;
        ekstazi::Function const & old_fun = p.second;

        auto it = new_functions.find(old_fun_name);

        // If not found, then we need to add this to the set of modified functions.
        if (it == new_functions.end())
        {
            modified_functions.insert(old_fun.name());
            continue;
        }

        // Compare the functions to see if their checksums differ
        ekstazi::Function const & new_fun = it->second;
        if (old_fun.checksum() != new_fun.checksum())
        {
            // errs() << "Checksums differ: " << old_fun_name << '\n';
            modified_functions.insert(old_fun.name());
        }
    }

    // Find new functions that were not in the old functions list
    for (std::pair<std::string, ekstazi::Function> const & p : new_functions)
    {
        std::string const & new_fun_name = p.first;
        ekstazi::Function const & new_fun = p.second;

        auto it = old_functions.find(new_fun_name);

        // If not found, then we need to add this to the set of modified functions.
        if (it == old_functions.end())
        {
            modified_functions.insert(new_fun.name());
        }
    }

    return modified_functions;
}

Function::Function(std::string const & name, std::string const & fname, std::string const & checksum) :
m_name { name },
m_filename { fname },
m_checksum { checksum }
{

}

std::string const & Function::name() const
{
    return m_name;
}

std::string const & Function::filename() const
{
    return m_filename;
}

std::string const & Function::checksum() const
{
    return m_checksum;
}

bool Function::operator==(Function const & fun) const
{
    return
        m_name == fun.m_name &&
        m_filename == fun.m_filename &&
        m_checksum == fun.m_checksum;
}

}

namespace std
{

size_t hash<ekstazi::Function>::operator()(ekstazi::Function const & fun) const
{
    return std::hash<std::string>{}(fun.checksum());
}

}
