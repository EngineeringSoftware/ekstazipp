
#include "ekstazi/depgraph/file-parser.hh"

#include <string>
#include <fstream>
#include <map>
#include <iostream>

namespace ekstazi
{

FileParser::FileParser(std::string const & fname) :
m_fname { fname }
{

}

void FileParser::parse_dependencies(DependencyGraph & graph, std::map<std::string, ekstazi::Function> & functions)
{
    std::cout << "Parsing dependency file: " << m_fname << std::endl;
    std::ifstream ifs{ m_fname };
    std::string line;
    std::string cur_fun;
    while (std::getline(ifs, line))
    {
        // search for the comma delimiter
        // std::cout << line << std::endl;
        std::size_t found = line.find(',');
        // if found, then it is a function declaration with the checksum
        // set our current function to the function declared in the line
        if (found != std::string::npos)
        {   // found a function
            cur_fun = line.substr(0, found);
            std::string checksum = line.substr(found + 1);
            std::cout << cur_fun << ", " << checksum << std::endl;
            functions.insert(std::make_pair(cur_fun, ekstazi::Function{ cur_fun, checksum, m_fname }));
            continue;
        }

        // not found, so the line is a dependency declaration for the current function
        std::string connected_fun = line;
        std::cout << "Dependency: " << connected_fun << " depended on by: " << cur_fun << std::endl;

        // the called function is depended on by the current function
        graph.add_dependency(connected_fun, cur_fun);
    }
}

}
