#include "storage.h"
#include "graph.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "tests/test_data"
#endif

int main(void) {
    Graph graph;
    Graph saved_graph;
    PlaceStore places;
    Node first = {10, "Start", 1.25, 2.5, NODE_GATE, 0.7f, 0.5f, 0.25f, 1};
    Node second = {20, "Goal", 7.0, 8.75, NODE_LANDMARK, 1.0f, 0.8f, 0.5f, 1};
    const char *temporary_map = "test_saved_map.tmp";
    assert(storage_load_graph(TEST_DATA_DIR "/nodes.csv", TEST_DATA_DIR "/edges.csv", &graph) == MSP_OK);
    assert(graph.node_count == 3);
    assert(graph.edge_count == 2);
    assert(storage_load_map(TEST_DATA_DIR "/map.txt", &graph) == MSP_OK);
    assert(graph.node_count == 3);
    assert(graph.edge_count == 2);
    assert(graph.nodes[0].type == NODE_GATE);
    assert(graph.nodes[1].type == NODE_JUNCTION);
    assert(graph.nodes[1].visible == 0);
    assert(fabs((double)graph.nodes[1].height - 0.03) < MSP_EPSILON);
    assert(storage_load_places(TEST_DATA_DIR "/places.csv", &graph, &places) == MSP_OK);
    assert(places.place_count == 2);
    assert(places.places[1].entrance_node_id == 3);
    assert(storage_find_place(&places, 2) == &places.places[1]);
    assert(storage_find_place_by_entrance(&places, 1) == &places.places[0]);
    assert(storage_load_map(TEST_DATA_DIR "/broken_map.txt", &graph) == MSP_ERROR_FORMAT);
    assert(storage_load_map(TEST_DATA_DIR "/missing_map.txt", &graph) == MSP_ERROR_IO);
    graph_init(&graph);
    assert(graph_add_node(&graph, first) == MSP_OK);
    assert(graph_add_node(&graph, second) == MSP_OK);
    assert(graph_add_edge(&graph, 10, 20, 3.75, 1) == MSP_OK);
    assert(storage_save_map(temporary_map, &graph) == MSP_OK);
    assert(storage_load_map(temporary_map, &saved_graph) == MSP_OK);
    assert(saved_graph.node_count == 2);
    assert(saved_graph.edge_count == 1);
    assert(saved_graph.nodes[0].type == NODE_GATE);
    assert(fabs((double)saved_graph.nodes[0].height - 0.25) < MSP_EPSILON);
    assert(graph_get_weight(&saved_graph, 0, 1) == 3.75);
    assert(storage_save_map(NULL, &graph) == MSP_ERROR_INVALID_ARGUMENT);
    assert(storage_save_map(temporary_map, NULL) == MSP_ERROR_INVALID_ARGUMENT);
    assert(storage_save_map("missing_directory/test_map.tmp", &graph) == MSP_ERROR_IO);
    assert(remove(temporary_map) == 0);
    puts("test_storage passed");
    return 0;
}
