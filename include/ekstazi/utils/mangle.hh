#pragma once

#include <string>
#include <cxxabi.h>

namespace ekstazi
{

std::string demangle(std::string const & name);

}
