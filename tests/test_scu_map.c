#include "astar.h"
#include "dijkstra.h"
#include "storage.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#ifndef PROJECT_DATA_DIR
#define PROJECT_DATA_DIR "data"
#endif

int main(void) {
    Graph graph;
    PathResult dijkstra_result;
    PathResult astar_result;

    assert(storage_load_map(PROJECT_DATA_DIR "/map.txt", &graph) == MSP_OK);
    assert(graph.node_count == 16);
    assert(graph.edge_count == 32);
    assert(dijkstra_find_path(&graph, 1, 13, &dijkstra_result) == MSP_OK);
    assert(astar_find_path(&graph, 1, 13, &astar_result) == MSP_OK);
    assert(fabs(dijkstra_result.total_distance - 15.2) < MSP_EPSILON);
    assert(fabs(astar_result.total_distance - 15.2) < MSP_EPSILON);
    puts("test_scu_map passed");
    return 0;
}
