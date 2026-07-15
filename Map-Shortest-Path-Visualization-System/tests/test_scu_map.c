#include "astar.h"
#include "dijkstra.h"
#include "graph.h"
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

static int is_road_node(NodeType type) {
    return type == NODE_GATE || type == NODE_JUNCTION || type == NODE_BRIDGE ||
           type == NODE_ENTRANCE || type == NODE_SERVICE;
}

int main(void) {
    Graph graph;
    PlaceStore places;
    PathResult dijkstra_result;
    PathResult astar_result;
    const int demo_route[] = {0, 27, 36, 33, 5, 34, 29, 35, 37, 40};
    const int east_to_library[] = {13, 32, 31, 37, 35, 29, 39};
    int junction_count = 0;
    int entrance_count = 0;
    int i;

    assert(storage_load_map(PROJECT_DATA_DIR "/map.txt", &graph) == MSP_OK);
    assert(graph.node_count == 45);
    assert(graph.edge_count == 26);
    for (i = 0; i < graph.node_count; ++i) {
        if (graph.nodes[i].type == NODE_JUNCTION) junction_count++;
        if (graph.nodes[i].type == NODE_ENTRANCE) entrance_count++;
    }
    assert(junction_count == 12);
    assert(entrance_count == 6);
    for (i = 0; i < graph.edge_count; ++i) {
        const Node *from = graph_get_node(&graph, graph.edges[i].from_id);
        const Node *to = graph_get_node(&graph, graph.edges[i].to_id);
        assert(from != NULL && to != NULL);
        assert(is_road_node(from->type));
        assert(is_road_node(to->type));
    }
    assert(storage_load_places(PROJECT_DATA_DIR "/places.csv", &graph, &places) == MSP_OK);
    assert(storage_find_place(&places, 0)->entrance_node_id == 0);
    assert(storage_find_place(&places, 2)->entrance_node_id == 40);
    assert(graph_get_weight(&graph, graph_find_node_index(&graph, 27),
                            graph_find_node_index(&graph, 30)) >= MSP_INFINITY);

    assert(dijkstra_find_path(&graph, 0, 40, &dijkstra_result) == MSP_OK);
    assert(astar_find_path(&graph, 0, 40, &astar_result) == MSP_OK);
    assert(fabs(dijkstra_result.total_distance - 13.3) < MSP_EPSILON);
    assert(fabs(astar_result.total_distance - 13.3) < MSP_EPSILON);
    assert_path(&dijkstra_result, demo_route,
                (int)(sizeof(demo_route) / sizeof(demo_route[0])));
    assert_path(&astar_result, demo_route,
                (int)(sizeof(demo_route) / sizeof(demo_route[0])));

    assert(dijkstra_find_path(&graph, 13, 39, &dijkstra_result) == MSP_OK);
    assert(astar_find_path(&graph, 13, 39, &astar_result) == MSP_OK);
    assert(fabs(dijkstra_result.total_distance - 8.4) < MSP_EPSILON);
    assert(fabs(astar_result.total_distance - 8.4) < MSP_EPSILON);
    assert_path(&dijkstra_result, east_to_library,
                (int)(sizeof(east_to_library) / sizeof(east_to_library[0])));
    assert_path(&astar_result, east_to_library,
                (int)(sizeof(east_to_library) / sizeof(east_to_library[0])));
    puts("test_scu_map passed");
    return 0;
}
