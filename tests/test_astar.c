#include "astar.h"

#include "graph.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    Graph graph;
    PathResult result;
    Node nodes[] = {{1, "A", 0, 0}, {2, "B", 1, 0}, {3, "C", 2, 0}, {4, "D", 0, 3}};
    int i;

    graph_init(&graph);
    for (i = 0; i < 4; ++i) assert(graph_add_node(&graph, nodes[i]) == MSP_OK);
    assert(graph_add_edge(&graph, 1, 2, 1.0, 1) == MSP_OK);
    assert(graph_add_edge(&graph, 2, 3, 1.0, 1) == MSP_OK);
    assert(graph_add_edge(&graph, 1, 4, 4.0, 1) == MSP_OK);
    assert(graph_add_edge(&graph, 4, 3, 4.0, 1) == MSP_OK);

    assert(astar_find_path(&graph, 1, 3, &result) == MSP_OK);
    assert(result.found && result.path_length == 3);
    assert(result.path[1] == 2);
    assert(fabs(result.total_distance - 2.0) < MSP_EPSILON);
    puts("test_astar passed");
    return 0;
}

