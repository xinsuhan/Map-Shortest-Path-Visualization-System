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

static int path_contains_external_node(const Graph *graph, const PathResult *result,
                                       const char *external_id) {
    int i;
    for (i = 0; i < result->path_length; ++i) {
        const Node *node = graph_get_node(graph, result->path[i]);
        if (node != NULL && strcmp(node->name, external_id) == 0) return 1;
    }
    return 0;
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

    assert(graph.node_count == 124);
    assert(graph.edge_count == 153);
    assert(places.place_count == 38);
    assert(graph.geometry_point_count == 373);
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
            "明远湖", "行政楼", "图书馆", "学术交流中心", "第一教学楼",
            "第二教学楼", "实践楼", "礼仪堂", "南门", "体育馆",
            "体育运动场", "游泳馆", "江安校区图书馆与博物馆", "文科楼",
            "学术文化交流中心", "东园一舍与二舍", "东园六舍", "东园七舍",
            "东园八舍", "东园留学生公寓与教师公寓", "教职工楼",
            "灾后重建与管理学院", "建筑与环境学院楼", "工程训练中心",
            "创新创业产业园", "东实学生食堂", "西园宿舍区", "西园食堂",
            "学生服务中心", "西南门", "西区体育场", "多学科交叉楼",
            "国家重点工程研究实验基地", "计算理论与未来研究院",
            "多学科交叉研究机构大楼", "江安新能源研究院", "东园食堂",
            "综合楼与一基楼"
        };
        const Place *library = find_place_by_name(&places, "图书馆");
        const Place *combined = find_place_by_name(&places, "综合楼与一基楼");
        const Node *display;
        const Node *entrance;
        size_t name_index;
        for (name_index = 0;
             name_index < sizeof(expected_names) / sizeof(expected_names[0]);
             ++name_index) {
            assert(find_place_by_name(&places, expected_names[name_index]) != NULL);
        }
        assert(library != NULL);
        display = graph_get_node(&graph, library->display_node_id);
        entrance = graph_get_node(&graph, library->entrance_node_id);
        assert(display != NULL && entrance != NULL);
        assert(fabs(display->x - 735.0) < MSP_EPSILON);
        assert(fabs(display->y - 385.0) < MSP_EPSILON);
        assert(fabs(entrance->x - 735.0) < MSP_EPSILON);
        assert(fabs(entrance->y - 302.0) < MSP_EPSILON);
        assert(combined != NULL);
        display = graph_get_node(&graph, combined->display_node_id);
        entrance = graph_get_node(&graph, combined->entrance_node_id);
        assert(display != NULL && entrance != NULL);
        assert(fabs(display->x - 950.0) < MSP_EPSILON);
        assert(fabs(display->y - 535.0) < MSP_EPSILON);
        assert(fabs(entrance->x - 895.0) < MSP_EPSILON);
        assert(fabs(entrance->y - 520.0) < MSP_EPSILON);
    }

    /* Regression: the route that previously circled through the whole west
     * dormitory must use the east/central roads visible on the new map. */
    {
        const Place *start = find_place_by_name(&places, "建筑与环境学院楼");
        const Place *goal = find_place_by_name(&places, "综合楼与一基楼");
        assert(start != NULL && goal != NULL);
        assert(dijkstra_find_path(&graph, start->entrance_node_id,
                                  goal->entrance_node_id,
                                  &dijkstra_result) == MSP_OK);
        assert(astar_find_path(&graph, start->entrance_node_id,
                               goal->entrance_node_id,
                               &astar_result) == MSP_OK);
        assert(dijkstra_result.found && astar_result.found);
        assert(dijkstra_result.total_distance < 700.0);
        assert(fabs(dijkstra_result.total_distance - astar_result.total_distance) < 1.0e-6);
        assert(!path_contains_external_node(&graph, &dijkstra_result, "J02"));
        assert(!path_contains_external_node(&graph, &dijkstra_result, "J27"));
        assert_path_uses_only_walkable_edges(&graph, &dijkstra_result);
        assert_path_uses_only_walkable_edges(&graph, &astar_result);
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
