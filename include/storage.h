#ifndef MAP_SHORTEST_PATH_STORAGE_H
#define MAP_SHORTEST_PATH_STORAGE_H

#include "models.h"

int storage_load_nodes(const char *file_path, Graph *graph);
int storage_load_edges(const char *file_path, Graph *graph, int bidirectional);
int storage_load_graph(const char *nodes_path, const char *edges_path, Graph *graph);
int storage_load_map(const char *file_path, Graph *graph);
int storage_save_map(const char *file_path, const Graph *graph);
int storage_save_path(const char *file_path, const Graph *graph, const PathResult *result);

#endif
