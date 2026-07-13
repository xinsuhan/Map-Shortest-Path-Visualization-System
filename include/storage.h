#ifndef MAP_SHORTEST_PATH_STORAGE_H
#define MAP_SHORTEST_PATH_STORAGE_H

#include "models.h"

const char *storage_node_type_name(NodeType type);
const char *storage_road_type_name(RoadType type);
int storage_load_nodes(const char *file_path, Graph *graph);
int storage_load_edges(const char *file_path, Graph *graph, int bidirectional);
int storage_load_graph(const char *nodes_path, const char *edges_path, Graph *graph);
int storage_load_curved_campus(const char *nodes_path, const char *edges_path,
                               const char *geometry_path, const char *pois_path,
                               Graph *graph, PlaceStore *places);
int storage_load_places(const char *file_path, const Graph *graph, PlaceStore *places);
const Place *storage_find_place(const PlaceStore *places, int place_id);
const Place *storage_find_place_by_display(const PlaceStore *places, int display_node_id);
const Place *storage_find_place_by_entrance(const PlaceStore *places, int entrance_node_id);
int storage_load_map(const char *file_path, Graph *graph);
int storage_save_map(const char *file_path, const Graph *graph);
int storage_save_path(const char *file_path, const Graph *graph, const PathResult *result);

#endif
