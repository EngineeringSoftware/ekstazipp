#pragma once

#include <string>

namespace ekstazi
{
// User directory for storing Ekstazi files
std::string const EKSTAZI_DIRNAME = ".ekstazi";

// Name of the type hierarchy file
std::string const TYPE_HIERARCHY_FNAME = "types.txt";

// Name for the count file
std::string const COUNT_FNAME = "count.ekstazi";

// Name for the dependnency graph file
std::string const DEPGRAPH_FNAME = "depgraph.txt";

// Name for the function checksums file
std::string const FUNCTIONS_FNAME = "functions.txt";

// Name for the constructor checksums file
std::string const CONSTRUCTORS_FNAME = "constructors.txt";

// Name for modified functions file
std::string const MODIFIED_FUNS_FNAME = "modified-functions.txt";

// Name for the modified tests file
std::string const TESTS_FNAME = "modified-tests.txt";

// Suffix for naming old files
std::string const OLD_SUFFIX = "old";

// Suffix for the modified functions file
std::string const MODIFIED_SUFFIX = "modified";

// Suffix for bitcode file
std::string const BC_SUFFIX = ".0.5.precodegen.bc";

}
