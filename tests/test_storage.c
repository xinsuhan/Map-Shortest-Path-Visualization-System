#include "storage.h"

#include <assert.h>
#include <stdio.h>

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "tests/test_data"
#endif

int main(void) {
    Graph graph;
    assert(storage_load_graph(TEST_DATA_DIR "/nodes.csv", TEST_DATA_DIR "/edges.csv", &graph) == MSP_OK);
    assert(graph.node_count == 3);
    assert(graph.edge_count == 2);
    assert(storage_load_map(TEST_DATA_DIR "/map.txt", &graph) == MSP_OK);
    assert(graph.node_count == 3);
    assert(graph.edge_count == 2);
    assert(storage_load_map(TEST_DATA_DIR "/broken_map.txt", &graph) == MSP_ERROR_FORMAT);
    assert(storage_load_map(TEST_DATA_DIR "/missing_map.txt", &graph) == MSP_ERROR_IO);
    puts("test_storage passed");
    return 0;
}
