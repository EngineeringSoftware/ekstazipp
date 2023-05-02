
#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <list>

namespace ekstazi
{

std::unordered_set<std::string> bfs(std::string const & start_node, std::unordered_map<std::string, std::list<std::string>> const & adj_list);

std::unordered_set<std::string> dfs(std::string const & start_node, std::unordered_map<std::string, std::list<std::string>> const & adj_list);
std::unordered_set<std::string> dfs(std::string const & start_node, std::unordered_map<std::string, std::list<std::string>> const & adj_list, std::unordered_set<std::string> const & visited);

/**
 * Returns the maximum distance of the graph between any 2 nodes.
 */
uint32_t max_distance(std::unordered_map<std::string, std::list<std::string>> const & adj_list);

/**
 * Returns the average distance of the graph from all leaf nodes.
 */
double average_distance(std::unordered_map<std::string, std::list<std::string>> const & adj_list);

/**
 * Returns the number of nodes in the graph.
 */
uint32_t num_nodes(std::unordered_map<std::string, std::list<std::string>> const & adj_list);

/**
 * Returns the number of non-root nodes in the graph.
 */
uint32_t num_nonroot_nodes(std::unordered_map<std::string, std::list<std::string>> const & adj_list);

}
