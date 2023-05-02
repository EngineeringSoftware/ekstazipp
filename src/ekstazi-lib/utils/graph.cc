
#include "ekstazi/utils/graph.hh"

#include <queue>

namespace ekstazi
{

std::unordered_set<std::string> bfs(std::string const & begin_node, std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    std::unordered_set<std::string> dependents{};

    // Conduct a breadth-first search
    std::queue<std::string> visit_queue{};
    std::unordered_set<std::string> visited{};

    visit_queue.push(begin_node);

    while (!visit_queue.empty())
    {
        // Get front element and mark as visited
        std::string cur_node = visit_queue.front();
        // std::cout << "Searching for children of " << cur_node << std::endl;
        visit_queue.pop();
        visited.insert(cur_node);

        // Get all direct dependents
        auto search = adj_list.find(cur_node);
        // If not found, continue
        if (search == adj_list.end())
        {
            // std::cout << "Not found in adjacency list: " << cur_node << std::endl;
            continue;
        }

        std::list<std::string> const & direct_dependents = search->second;
        dependents.insert(direct_dependents.begin(), direct_dependents.end());

        // Add to visit_queue queue if not already visited
        // std::cout << "Direct dependents of " << cur_node << ": " << std::endl;
        for (std::string const & dependent : direct_dependents)
        {
            // std::cout << dependent << std::endl;
            auto search = visited.find(dependent);
            if (search == visited.end())
            {
                visit_queue.push(dependent);
                // std::cout << "Added " << dependent << " to the visit queue." << std::endl;
            }
        }
    }

    return dependents;
}

std::unordered_set<std::string> dfs(std::string const & begin_node, std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    std::unordered_set<std::string> dependents{};

    // Conduct a breadth-first search
    std::queue<std::string> visit_queue{};
    std::unordered_set<std::string> visited{};

    visit_queue.push(begin_node);

    while (!visit_queue.empty())
    {
        // Get front element and mark as visited
        std::string cur_node = visit_queue.front();
        // std::cout << "Searching for children of " << cur_node << std::endl;
        visit_queue.pop();
        visited.insert(cur_node);

        // Get all direct dependents
        auto search = adj_list.find(cur_node);
        // If not found, continue
        if (search == adj_list.end())
        {
            // std::cout << "Not found in adjacency list: " << cur_node << std::endl;
            continue;
        }

        std::list<std::string> const & direct_dependents = search->second;
        dependents.insert(direct_dependents.begin(), direct_dependents.end());

        // Add to visit_queue queue if not already visited
        // std::cout << "Direct dependents of " << cur_node << ": " << std::endl;
        for (std::string const & dependent : direct_dependents)
        {
            // std::cout << dependent << std::endl;
            auto search = visited.find(dependent);
            if (search == visited.end())
            {
                visit_queue.push(dependent);
                // std::cout << "Added " << dependent << " to the visit queue." << std::endl;
            }
        }
    }

    return dependents;
}

/**
 * Returns the max distance of a BFS search from a node.
 */
uint32_t max_distance_bfs(std::string const & begin_node, std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    // Conduct a breadth-first search
    std::queue<std::string> visit_queue{};
    std::unordered_set<std::string> visited{};

    // Map of nodes to their distance from begin_node
    std::unordered_map<std::string, uint32_t> distances;

    visit_queue.push(begin_node);
    distances[begin_node] = 0;

    while (!visit_queue.empty())
    {
        // Get front element and mark as visited
        std::string cur_node = visit_queue.front();
        // std::cout << "Searching for children of " << cur_node << std::endl;
        visit_queue.pop();
        visited.insert(cur_node);

        // Save the current distance from begin_node
        uint32_t cur_distance = distances[cur_node];

        // Get all direct dependents
        auto search = adj_list.find(cur_node);
        // If not found, continue
        if (search == adj_list.end())
        {
            // std::cout << "Not found in adjacency list: " << cur_node << std::endl;
            continue;
        }

        std::list<std::string> const & direct_dependents = search->second;

        // Add to visit_queue queue if not already visited
        // std::cout << "Direct dependents of " << cur_node << ": " << std::endl;
        for (std::string const & dependent : direct_dependents)
        {
            // std::cout << dependent << std::endl;
            auto search = visited.find(dependent);
            if (search == visited.end())
            {
                visit_queue.push(dependent);
                distances[dependent] = cur_distance + 1;
                // std::cout << "Added " << dependent << " to the visit queue." << std::endl;
            }
        }
    }

    uint32_t max_distance = 0;
    for (std::pair<std::string, uint32_t> const & p : distances)
    {
        if (p.second > max_distance)
        {
            max_distance = p.second;
        }
    }

    return max_distance;
}

/**
 * Returns the leaf nodes in the graph.
 */
std::unordered_set<std::string> find_leaf_nodes(std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    std::unordered_set<std::string> leaf_nodes;
    // Find all nodes in graph first and filter by leaves.
    // Leaves are nodes where either the adjacency list key doesn't exist, or
    // it does exist and the adjacency list for that node is empty
    for (std::pair<std::string, std::list<std::string>> const & p : adj_list)
    {
        // Is a leaf if the list is empty.
        if (p.second.empty())
        {
            leaf_nodes.insert(p.first);
        }
        
        for (std::string const & adjacent_node : p.second)
        {
            std::unordered_map<std::string, std::list<std::string>>::const_iterator it = adj_list.find(adjacent_node);
            // Is a leaf if the entry doesn't exist in the adjacency list
            if (it == adj_list.end())
            {
                leaf_nodes.insert(adjacent_node);
                continue;
            }

            // Is also a leaf if the entry does exist but the adjacency list is empty.
            if (it->second.empty())
            {
                leaf_nodes.insert(adjacent_node);
                continue;
            }
        }
    }

    return leaf_nodes;
}

/**
 * Reverses all edges in the graph.
 */
std::unordered_map<std::string, std::list<std::string>> reverse_edges(std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    std::unordered_map<std::string, std::list<std::string>> reversed_adj_list;
    for (std::pair<std::string, std::list<std::string>> const & p : adj_list)
    {
        for (std::string const & adjacent_node : p.second)
        {
            // Insert the edge but reversed
            reversed_adj_list[adjacent_node].push_back(p.first);
        }
    }

    return reversed_adj_list;
}

/**
 * Returns the maximum distance of the graph between any 2 nodes.
 */
uint32_t max_distance(std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    std::vector<uint32_t> distances;
    // Find max distances from all nodes
    for (std::pair<std::string, std::list<std::string>> const & p : adj_list)
    {
        distances.push_back(max_distance_bfs(p.first, adj_list));
    }

    uint32_t max_distance = 0;
    for (uint32_t distance : distances)
    {
        if (distance > max_distance)
        {
            max_distance = distance;
        }
    }

    return max_distance;
}

/**
 * Returns the average distance of the graph from all leaf nodes.
 */
double average_distance(std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    // Find all leaf nodes
    std::unordered_set<std::string> leaf_nodes = find_leaf_nodes(adj_list);

    // Now reverse the edges in the graph and find the max distance for each leaf node
    std::unordered_map<std::string, std::list<std::string>> reversed_adj_list = reverse_edges(adj_list);

    // Find max distances for each leaf node
    std::vector<uint32_t> distances;
    
    for (std::string const & leaf_node : leaf_nodes)
    {
        distances.push_back(max_distance_bfs(leaf_node, reversed_adj_list));
    }

    double average_distance = 0;
    for (uint32_t distance : distances)
    {
        average_distance += distance;
    }

    average_distance /= distances.size();

    return average_distance;
}

/**
 * Returns the number of nodes in the graph.
 */
uint32_t num_nodes(std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    std::unordered_set<std::string> nodes;
    for (std::pair<std::string, std::list<std::string>> const & p : adj_list)
    {
        nodes.insert(p.first);
        for (std::string const & adj_node : p.second)
        {
            nodes.insert(adj_node);
        }
    }

    return nodes.size();
}

/**
 * Returns the number of non-root nodes in the graph.
 */
uint32_t num_nonroot_nodes(std::unordered_map<std::string, std::list<std::string>> const & adj_list)
{
    uint32_t total_nodes = num_nodes(adj_list);
    // First we reverse the graph, then count the number of non-leaf nodes.
    std::unordered_map<std::string, std::list<std::string>> rev_adj_list = reverse_edges(adj_list);

    std::unordered_set<std::string> leaf_nodes = find_leaf_nodes(rev_adj_list);

    return total_nodes - leaf_nodes.size();
}

}