#include "path_result.h"
#include "graph.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    int i;
    PathResult *result = path_result_create();
    Graph graph;
    int previous[MSP_MAX_NODES];

    assert(result != NULL);
    assert(result->found == 0);
    assert(result->total_distance == 0.0);
    assert(result->path_length == 0);
    assert(result->visited_count == 0);
    for (i = 0; i < MSP_MAX_NODES; ++i) {
        assert(result->path[i] == 0);
        assert(result->visited_order[i] == 0);
        previous[i] = -1;
    }
    path_result_reset(result);
    assert(result->total_distance == MSP_INFINITY);
    assert(result->path[0] == -1);
    assert(path_result_build(NULL, 0, previous, result) == MSP_ERROR_INVALID_ARGUMENT);
    graph_init(&graph);
    assert(graph_add_node(&graph, (Node){.id = 10, .name = "A"}) == MSP_OK);
    assert(graph_add_node(&graph, (Node){.id = 20, .name = "B"}) == MSP_OK);
    previous[1] = 0;
    assert(path_result_build(&graph, 1, previous, result) == MSP_OK);
    assert(result->path_length == 2 && result->path[0] == 10 && result->path[1] == 20);
    previous[1] = 1;
    assert(path_result_build(&graph, 1, previous, result) == MSP_ERROR_FORMAT);
    assert(result->path_length == 0);
    path_result_destroy(result);
    path_result_destroy(NULL);
    puts("test_path_result passed");
    return 0;
}
