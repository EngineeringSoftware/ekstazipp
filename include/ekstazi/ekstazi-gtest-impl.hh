
#include <sstream>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <string>

namespace ekstazi
{

std::string get_gtest_filter()
{
    // First check if ekstazi has been run yet
    std::string count_fname = ".ekstazi/count.ekstazi";
    std::ifstream ifs{ count_fname };
    std::string line;
    std::getline(ifs, line);
    std::stringstream ss{ line };
    int count;
    ss >> count;

    // First time, so return empty filter
    if (count == 1)
    {
        ifs.close();
        return "*";
    }

    ifs.close();

    std::string fname = ".ekstazi/modified-tests.txt";

    std::string gtest_filter{};

    ifs = std::ifstream{ fname };
    bool first = true;
    while (std::getline(ifs, line))
    {
        // Remove namespaces from the test name
        size_t index = line.find_last_of("::");
        if (index != std::string::npos)
        {
            line = line.substr(index + 1);
        }

        if (first)
        {
            gtest_filter += line;
            first = false;
            continue;
        }
        gtest_filter += ":" + line;
    }

    std::cout << "Gtest filter is: " << gtest_filter << std::endl;

    return gtest_filter;
}



}
