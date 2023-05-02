
#include "ekstazi/type-hierarchy/type-hierarchy.hh"

#include <string>
#include <fstream>
#include <iostream>

using namespace ekstazi;

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Incorrect number of inputs. Expected 1 but got " << argc - 1 << std::endl;
        exit(1);
    }
    std::string input_fname = argv[1];

    std::ifstream ifs{input_fname};
    if (!ifs)
    {
        std::cerr << "File not found: " << input_fname << std::endl;
        exit(1);
    }

    TypeHierarchy th{};
    th.load_file(input_fname);

    std::cout << "num_types: " << th.size() << std::endl;
    std::cout << "num_derived_types: " << th.num_derived_types() << std::endl;
    std::cout << "max_depth: " << th.max_depth() << std::endl;
    std::cout << "avg_depth: " << th.average_depth() << std::endl;
}
