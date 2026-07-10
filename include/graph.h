#ifndef MAP_SHORTEST_PATH_GRAPH_H
#define MAP_SHORTEST_PATH_GRAPH_H

#include "models.h"

void graph_init(Graph *graph);
int graph_add_node(Graph *graph, Node node);
int graph_add_edge(Graph *graph, int from_id, int to_id, double weight, int bidirectional);
int graph_add_road_edge(Graph *graph, int from_id, int to_id, double weight,
                        RoadType type, int walkable, int bidirectional);
int graph_find_node_index(const Graph *graph, int node_id);
const Node *graph_get_node(const Graph *graph, int node_id);
double graph_get_weight(const Graph *graph, int from_index, int to_index);

#endif

