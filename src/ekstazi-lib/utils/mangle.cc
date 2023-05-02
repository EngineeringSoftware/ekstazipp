
#include "ekstazi/utils/mangle.hh"

#include <memory>

namespace ekstazi
{

std::string demangle(std::string const & name)
{
    int status = -1;

    std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name.c_str(), NULL, NULL, &status), std::free };
    return (status == 0) ? res.get() : std::string(name);
}

}
