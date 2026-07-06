#include "graph.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    Graph graph;
    Node first = {1, "A", 0.0, 0.0};
    Node second = {2, "B", 1.0, 0.0};

    graph_init(&graph);
    assert(graph_add_node(&graph, first) == MSP_OK);
    assert(graph_add_node(&graph, second) == MSP_OK);
    assert(graph_add_node(&graph, first) == MSP_ERROR_DUPLICATE);
    assert(graph_add_edge(&graph, 1, 2, 3.5, 1) == MSP_OK);
    assert(graph_get_weight(&graph, 0, 1) == 3.5);
    assert(graph_get_weight(&graph, 1, 0) == 3.5);
    assert(strcmp(graph_get_node(&graph, 2)->name, "B") == 0);
    puts("test_graph passed");
    return 0;
}

