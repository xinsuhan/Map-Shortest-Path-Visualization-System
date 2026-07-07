#include "astar.h"
#include "dijkstra.h"
#include "storage.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#ifndef PROJECT_DATA_DIR
#define PROJECT_DATA_DIR "data"
#endif

static void assert_path(const PathResult *result, const int expected[], int expected_length) {
    int i;
    assert(result->path_length == expected_length);
    for (i = 0; i < expected_length; ++i) {
        assert(result->path[i] == expected[i]);
    }
}

int main(void) {
    Graph graph;
    PathResult dijkstra_result;
    PathResult astar_result;
    const int route_one[] = {1, 0, 27, 30, 31, 32, 13};
    const int route_three[] = {4, 27, 28, 29, 7, 10};

    assert(storage_load_map(PROJECT_DATA_DIR "/map.txt", &graph) == MSP_OK);
    assert(graph.node_count == 35);
    assert(graph.edge_count == 73);

    assert(dijkstra_find_path(&graph, 1, 13, &dijkstra_result) == MSP_OK);
    assert(astar_find_path(&graph, 1, 13, &astar_result) == MSP_OK);
    assert(fabs(dijkstra_result.total_distance - 12.8) < MSP_EPSILON);
    assert(fabs(astar_result.total_distance - 12.8) < MSP_EPSILON);
    assert_path(&dijkstra_result, route_one,
                (int)(sizeof(route_one) / sizeof(route_one[0])));
    assert_path(&astar_result, route_one,
                (int)(sizeof(route_one) / sizeof(route_one[0])));

    assert(dijkstra_find_path(&graph, 4, 10, &dijkstra_result) == MSP_OK);
    assert(astar_find_path(&graph, 4, 10, &astar_result) == MSP_OK);
    assert(fabs(dijkstra_result.total_distance - 10.3) < MSP_EPSILON);
    assert(fabs(astar_result.total_distance - 10.3) < MSP_EPSILON);
    assert_path(&dijkstra_result, route_three,
                (int)(sizeof(route_three) / sizeof(route_three[0])));
    assert_path(&astar_result, route_three,
                (int)(sizeof(route_three) / sizeof(route_three[0])));
    puts("test_scu_map passed");
    return 0;
}
