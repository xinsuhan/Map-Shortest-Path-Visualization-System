#include "astar.h"
#include "dijkstra.h"
#include "graph.h"
#include "storage.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef PROJECT_DATA_DIR
#define PROJECT_DATA_DIR "data"
#endif

static int is_road_node(NodeType type) {
    return type == NODE_JUNCTION || type == NODE_GATE || type == NODE_BRIDGE ||
           type == NODE_ENTRANCE || type == NODE_SERVICE;
}

static const Place *find_place_by_name(const PlaceStore *places, const char *name) {
    int i;
    for (i = 0; i < places->place_count; ++i) {
        if (strcmp(places->places[i].name, name) == 0) return &places->places[i];
    }
    return NULL;
}

static void assert_path_uses_only_walkable_edges(const Graph *graph,
                                                 const PathResult *result) {
    int path_index;
    int edge_index;
    for (path_index = 0; path_index + 1 < result->path_length; ++path_index) {
        int from_id = result->path[path_index];
        int to_id = result->path[path_index + 1];
        int from_index = graph_find_node_index(graph, from_id);
        int to_index = graph_find_node_index(graph, to_id);
        assert(graph_get_weight(graph, from_index, to_index) < MSP_INFINITY);
        for (edge_index = 0; edge_index < graph->edge_count; ++edge_index) {
            const Edge *edge = &graph->edges[edge_index];
            if (!edge->walkable) {
                assert(!((edge->from_id == from_id && edge->to_id == to_id) ||
                         (edge->from_id == to_id && edge->to_id == from_id)));
            }
        }
    }
}

int main(void) {
    Graph graph;
    PlaceStore places;
    PathResult dijkstra_result;
    PathResult astar_result;
    int load_status;
    int disabled_edge_count = 0;
    int i;
    int j;

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
        const Edge *edge = &graph.edges[i];
        assert(graph.edges[i].geometry_start >= 0);
        assert(graph.edges[i].geometry_count >= 2);
        assert(graph.edges[i].geometry_start + graph.edges[i].geometry_count <=
               graph.geometry_point_count);
        if (!edge->walkable) {
            int from_index = graph_find_node_index(&graph, edge->from_id);
            int to_index = graph_find_node_index(&graph, edge->to_id);
            disabled_edge_count++;
            assert(graph_get_weight(&graph, from_index, to_index) >= MSP_INFINITY);
            assert(graph_get_weight(&graph, to_index, from_index) >= MSP_INFINITY);
        }
    }
    assert(disabled_edge_count > 0);

    for (i = 0; i < places.place_count; ++i) {
        const Place *place = &places.places[i];
        const Node *display = graph_get_node(&graph, place->display_node_id);
        const Node *entrance = graph_get_node(&graph, place->entrance_node_id);
        int anchor_edge_count = 0;
        assert(display != NULL);
        assert(entrance != NULL);
        assert(is_road_node(entrance->type));
        assert(place->display_node_id != place->entrance_node_id);
        if (strcmp(place->category, "gate") != 0) {
            assert(fabs(display->x - entrance->x) > MSP_EPSILON ||
                   fabs(display->y - entrance->y) > MSP_EPSILON);
        }
        assert(storage_find_place_by_display(&places, place->display_node_id) == place);
        assert(storage_find_place_by_entrance(&places, place->entrance_node_id) == place);
        for (j = 0; j < graph.edge_count; ++j) {
            const Edge *edge = &graph.edges[j];
            if (edge->from_id == place->display_node_id ||
                edge->to_id == place->display_node_id) {
                int display_index = graph_find_node_index(&graph, place->display_node_id);
                int entrance_index = graph_find_node_index(&graph, place->entrance_node_id);
                assert(!edge->walkable);
                assert(edge->type == ROAD_ENTRANCE);
                assert((edge->from_id == place->display_node_id &&
                        edge->to_id == place->entrance_node_id) ||
                       (edge->to_id == place->display_node_id &&
                        edge->from_id == place->entrance_node_id));
                assert(edge->geometry_count == 2);
                assert(graph_get_weight(&graph, display_index, entrance_index) >= MSP_INFINITY);
                assert(graph_get_weight(&graph, entrance_index, display_index) >= MSP_INFINITY);
                anchor_edge_count++;
            }
        }
        assert(anchor_edge_count == 1);
    }

    {
        static const char *const expected_names[] = {
            "Mingyuan Lake", "Arts College", "Campus Stadium", "West Dormitory",
            "East Dormitory", "Disaster Reconstruction and Management College",
            "Southwest Gate", "Library", "Architecture and Environment College Building",
            "Second Basic Building", "East Canteen", "West Canteen", "Gymnasium",
            "Liberal Arts Buildings", "Aerospace Building", "First Teaching Building",
            "Comprehensive and First Basic Buildings", "Administration Building"
        };
        static const char *const removed_names[] = {
            "East Teaching Area", "First Basic Building", "Engineering Training Center",
            "Arts and Sciences College", "Comprehensive Building", "Campus Hospital",
            "Knowledge Square"
        };
        const Place *arts = find_place_by_name(&places, "Arts College");
        const Place *combined = find_place_by_name(
            &places, "Comprehensive and First Basic Buildings");
        const Node *display;
        const Node *entrance;
        size_t name_index;
        for (name_index = 0;
             name_index < sizeof(expected_names) / sizeof(expected_names[0]);
             ++name_index) {
            assert(find_place_by_name(&places, expected_names[name_index]) != NULL);
        }
        for (name_index = 0;
             name_index < sizeof(removed_names) / sizeof(removed_names[0]);
             ++name_index) {
            assert(find_place_by_name(&places, removed_names[name_index]) == NULL);
        }
        assert(arts != NULL);
        display = graph_get_node(&graph, arts->display_node_id);
        entrance = graph_get_node(&graph, arts->entrance_node_id);
        assert(display != NULL && entrance != NULL);
        assert(fabs(display->x - 580.0) < MSP_EPSILON);
        assert(fabs(display->y - 710.0) < MSP_EPSILON);
        assert(fabs(entrance->x - 580.0) < MSP_EPSILON);
        assert(fabs(entrance->y - 670.0) < MSP_EPSILON);
        assert(combined != NULL);
        display = graph_get_node(&graph, combined->display_node_id);
        entrance = graph_get_node(&graph, combined->entrance_node_id);
        assert(display != NULL && entrance != NULL);
        assert(fabs(display->x - 1040.0) < MSP_EPSILON);
        assert(fabs(display->y - 790.0) < MSP_EPSILON);
        assert(fabs(entrance->x - 1050.0) < MSP_EPSILON);
        assert(fabs(entrance->y - 806.0) < MSP_EPSILON);
    }

    for (i = 0; i < places.place_count; ++i) {
        for (j = i + 1; j < places.place_count; ++j) {
            const Place *start = &places.places[i];
            const Place *goal = &places.places[j];
            assert(start->entrance_node_id != goal->entrance_node_id);
            assert(dijkstra_find_path(&graph, start->entrance_node_id,
                                      goal->entrance_node_id,
                                      &dijkstra_result) == MSP_OK);
            assert(astar_find_path(&graph, start->entrance_node_id,
                                   goal->entrance_node_id,
                                   &astar_result) == MSP_OK);
            assert(dijkstra_result.found);
            assert(astar_result.found);
            assert(dijkstra_result.path[0] == start->entrance_node_id);
            assert(dijkstra_result.path[dijkstra_result.path_length - 1] ==
                   goal->entrance_node_id);
            assert(dijkstra_result.path[0] != start->display_node_id);
            assert(dijkstra_result.path[dijkstra_result.path_length - 1] !=
                   goal->display_node_id);
            assert(fabs(dijkstra_result.total_distance -
                        astar_result.total_distance) < 1.0e-6);
            assert_path_uses_only_walkable_edges(&graph, &dijkstra_result);
            assert_path_uses_only_walkable_edges(&graph, &astar_result);
        }
    }

    puts("test_curved_campus passed");
    return 0;
}
