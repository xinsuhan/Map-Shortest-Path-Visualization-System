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
    PlaceStore places;
    PathResult dijkstra_result;
    PathResult astar_result;
    int start_id;
    int load_status;
    int i;

    load_status = storage_load_curved_campus(
        PROJECT_DATA_DIR "/curved/nodes.csv",
        PROJECT_DATA_DIR "/curved/edges_curved.csv",
        PROJECT_DATA_DIR "/curved/edge_geometry_points.csv",
        PROJECT_DATA_DIR "/curved/pois.csv", &graph, &places);
    if (load_status != MSP_OK) {
        fprintf(stderr, "curved campus load failed: %d\n", load_status);
    }
    assert(load_status == MSP_OK);

    assert(graph.node_count == 122);
    assert(graph.edge_count == 178);
    assert(places.place_count == 18);
    assert(graph.geometry_point_count == 801);
    assert(graph.weights_in_meters == 1);

    for (i = 0; i < graph.edge_count; ++i) {
        assert(graph.edges[i].geometry_start >= 0);
        assert(graph.edges[i].geometry_count >= 2);
        assert(graph.edges[i].geometry_start + graph.edges[i].geometry_count <=
               graph.geometry_point_count);
    }

    start_id = places.places[0].entrance_node_id;
    for (i = 1; i < places.place_count; ++i) {
        int goal_id = places.places[i].entrance_node_id;
        assert(dijkstra_find_path(&graph, start_id, goal_id,
                                  &dijkstra_result) == MSP_OK);
        assert(astar_find_path(&graph, start_id, goal_id,
                               &astar_result) == MSP_OK);
        assert(dijkstra_result.found);
        assert(astar_result.found);
        assert(fabs(dijkstra_result.total_distance -
                    astar_result.total_distance) < 1.0e-6);
    }

    puts("test_curved_campus passed");
    return 0;
}
