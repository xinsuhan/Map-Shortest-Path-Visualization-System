#include "graph.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    Graph graph;
    Node first = {1, "A", 0.0, 0.0, NODE_LANDMARK, 1.0f, 0.8f, 0.5f, 1};
    Node second = {2, "B", 1.0, 0.0, NODE_LANDMARK, 1.0f, 0.8f, 0.5f, 1};

    graph_init(&graph);
    assert(graph_add_node(&graph, first) == MSP_OK);
    assert(graph_add_node(&graph, second) == MSP_OK);
    assert(graph_add_node(&graph, first) == MSP_ERROR_DUPLICATE);
    assert(graph_add_edge(&graph, 1, 2, 3.5, 1) == MSP_OK);
    assert(graph_get_weight(&graph, 0, 1) == 3.5);
    assert(graph_get_weight(&graph, 1, 0) == 3.5);
    assert(graph_add_edge(&graph, 1, 2, 4.0, 0) == MSP_ERROR_DUPLICATE);
    assert(graph_add_edge(&graph, 2, 1, 4.0, 1) == MSP_ERROR_DUPLICATE);
    assert(graph.edge_count == 1);
    assert(strcmp(graph_get_node(&graph, 2)->name, "B") == 0);
    graph_init(&graph);
    assert(graph_add_node(&graph, first) == MSP_OK);
    assert(graph_add_node(&graph, second) == MSP_OK);
    assert(graph_add_road_edge(&graph, 1, 2, 2.0, ROAD_BRANCH, 0, 1) == MSP_OK);
    assert(graph.edge_count == 1);
    assert(graph.edges[0].walkable == 0);
    assert(graph_get_weight(&graph, 0, 1) >= MSP_INFINITY);
    assert(graph_add_road_edge(&graph, 1, 1, 1.0, ROAD_MAIN, 1, 1) ==
           MSP_ERROR_INVALID_ARGUMENT);
    assert(graph_add_road_edge(&graph, 1, 2, 0.0, ROAD_MAIN, 1, 1) ==
           MSP_ERROR_INVALID_ARGUMENT);
    puts("test_graph passed");
    return 0;
}

