#pragma once

#include <string>
#include <map>

#include "depgraph.hh"
#include "function.hh"

namespace ekstazi
{

class FileParser
{
public:
    FileParser(std::string const & fname);
    void parse_dependencies(DependencyGraph & graph, std::map<std::string, ekstazi::Function> & hashes);

protected:
    std::string m_fname;
};

}
