#include "astar.h"
#include "dijkstra.h"
#include "graph.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    Graph graph;
    PathResult result;
    Node first = {1, "A", 0.0, 0.0, NODE_LANDMARK, 1.0f, 0.8f, 0.5f, 1};
    Node second = {2, "B", 1.0, 0.0, NODE_LANDMARK, 1.0f, 0.8f, 0.5f, 1};
    Node isolated = {3, "C", 5.0, 5.0, NODE_LANDMARK, 1.0f, 0.8f, 0.5f, 1};

    graph_init(&graph);
    assert(graph_add_node(&graph, first) == MSP_OK);
    assert(graph_add_node(&graph, second) == MSP_OK);
    assert(graph_add_node(&graph, isolated) == MSP_OK);
    assert(graph_add_edge(&graph, 1, 2, 1.0, 1) == MSP_OK);

    assert(dijkstra_find_path(&graph, 1, 1, &result) == MSP_ERROR_INVALID_ARGUMENT);
    assert(astar_find_path(&graph, 1, 1, &result) == MSP_ERROR_INVALID_ARGUMENT);
    assert(dijkstra_find_path(&graph, 1, 3, &result) == MSP_ERROR_NO_PATH);
    assert(astar_find_path(&graph, 1, 3, &result) == MSP_ERROR_NO_PATH);
    assert(dijkstra_find_path(&graph, 99, 1, &result) == MSP_ERROR_NOT_FOUND);
    assert(astar_find_path(&graph, 1, 99, &result) == MSP_ERROR_NOT_FOUND);
    assert(graph_add_edge(&graph, 1, 2, -1.0, 1) == MSP_ERROR_INVALID_ARGUMENT);

    puts("test_edge_cases passed");
    return 0;
}
