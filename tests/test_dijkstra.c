#include "dijkstra.h"

#include "graph.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    Graph graph;
    PathResult result;
    Node nodes[] = {{.id = 1, .name = "A", .x = 0, .y = 0},
                    {.id = 2, .name = "B", .x = 1, .y = 0},
                    {.id = 3, .name = "C", .x = 2, .y = 0},
                    {.id = 4, .name = "D", .x = 1, .y = 2}};
    int i;

    graph_init(&graph);
    for (i = 0; i < 4; ++i) assert(graph_add_node(&graph, nodes[i]) == MSP_OK);
    assert(graph_add_edge(&graph, 1, 2, 1.0, 1) == MSP_OK);
    assert(graph_add_edge(&graph, 2, 3, 1.0, 1) == MSP_OK);
    assert(graph_add_edge(&graph, 1, 4, 5.0, 1) == MSP_OK);
    assert(graph_add_edge(&graph, 4, 3, 5.0, 1) == MSP_OK);

    assert(dijkstra_find_path(&graph, 1, 3, &result) == MSP_OK);
    assert(result.found && result.path_length == 3);
    assert(result.path[0] == 1 && result.path[1] == 2 && result.path[2] == 3);
    assert(fabs(result.total_distance - 2.0) < MSP_EPSILON);
    puts("test_dijkstra passed");
    return 0;
}

